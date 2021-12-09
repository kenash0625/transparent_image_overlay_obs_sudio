/*************************************************************************
 * This file is part of input-overlay
 * github.con/univrsal/input-overlay
 * Copyright 2021 univrsal <uni@vrsal.de>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************/

#include "overlay.hpp"
#include "../sources/input_source.hpp"
#include "config.hpp"
#include "log.h"

#include <QFile>
#include <QImage>
#include <QPainter>
#include <QJsonArray>
#include <QJsonDocument>
#include<iostream>
#include<QImage>
extern "C" {
#include <graphics/image-file.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}


#define ERROR_BUFSIZ 1024
char errorstring[ERROR_BUFSIZ];

uint8_t * gs_create_texture_file_data2_test1(
	const char* file,
	enum gs_image_alpha_mode alpha_mode,
	enum gs_color_format* format,
	uint32_t * cx_out, uint32_t * cy_out)
{
	QImage cropped = *(QImage*)file;
	// Make sure that your image and this->rect have the same dimensions

	// Odd numbered GIF widths/heights are known to cause errors in the YUV420P encoding process making the GIF slightly yellow.
	// so the extra pixel length/width is chopped
	const qint32 width = cropped.width() & (~1);
	const qint32 height = cropped.height() & (~1);

	*cx_out = width;
	*cy_out = height;
	*format = GS_BGRA;

	AVFrame* tmpFrame = av_frame_alloc();
	tmpFrame->width = /*this->codecContext->*/ width;
	tmpFrame->height = /*this->codecContext->*/ height;
	tmpFrame->format = AV_PIX_FMT_BGRA;

	auto error = av_frame_get_buffer(tmpFrame, 0);
	if (error < 0) {
		av_strerror(error, errorstring, ERROR_BUFSIZ);
		std::cerr << __FILE__ << ":" << __LINE__ << " " << errorstring
			<< std::endl;
		return nullptr;
	}

	error = av_image_alloc(tmpFrame->data, tmpFrame->linesize, width,
		height, AV_PIX_FMT_BGRA, 32);
	if (error < 0) {
		av_strerror(error, errorstring, ERROR_BUFSIZ);
		std::cerr << __FILE__ << ":" << __LINE__ << " " << errorstring
			<< std::endl;
		return nullptr;
	}

	// When we pass a frame to the encoder, it may keep a reference to it internally;
	// make sure we do not overwrite it here!
	error = av_frame_make_writable(tmpFrame);
	if (error < 0) {
		av_strerror(error, errorstring, ERROR_BUFSIZ);
		std::cerr << __FILE__ << ":" << __LINE__ << " " << errorstring
			<< std::endl;
		return nullptr;
	}

	// Converting QImage to AV_PIX_FMT_BGRA AVFrame ...
	for (qint32 y = 0; y < height; y++) {
		const uint8_t* scanline = cropped.scanLine(y);

		for (qint32 x = 0; x < width * 4; x++) {
			tmpFrame->data[0][y * tmpFrame->linesize[0] + x] =
				scanline[x];
		}
	}

	const size_t linesize = (size_t)width * 4;
	const size_t totalsize = height * linesize;
	void* data = bmalloc(totalsize);

	const size_t src_linesize = tmpFrame->linesize[0];
	if (linesize != src_linesize) {
		const size_t min_line = linesize < src_linesize ? linesize
			: src_linesize;

		uint8_t* dst = (uint8_t*)data;
		const uint8_t* src = (uint8_t*)tmpFrame->data[0];
		for (int y = 0; y < height; y++) {
			memcpy(dst, src, min_line);
			dst += linesize;
			src += src_linesize;
		}
	}
	else {
		memcpy(data, tmpFrame->data[0], totalsize);
	}
	av_freep(&tmpFrame->data[0]);
	av_frame_free(&tmpFrame);
	return (uint8_t*)data;
}

void gs_image_file_init_test1(gs_image_file_t* image, const char* file)
{
    memset(image, 0, sizeof(*image));
    image->texture_data = gs_create_texture_file_data2_test1(
        file, GS_IMAGE_ALPHA_STRAIGHT, &image->format, &image->cx,
        &image->cy);
    image->loaded = !!image->texture_data;
}
void gs_image_file_free_test1(gs_image_file_t* image)
{
    if (!image)
        return;

    if (image->loaded) {
        gs_texture_destroy(image->texture);
    }

    bfree(image->texture_data);
    bfree(image->gif_data);
    memset(image, 0, sizeof(*image));
}
namespace sources {
class overlay_settings;
}

overlay::~overlay()
{
    unload();
}

overlay::overlay(sources::overlay_settings *settings)
{
    m_settings = settings;
    m_is_loaded = load();
}

bool overlay::load()
{
    unload();
    const auto image_loaded = load_texture();
    m_is_loaded = image_loaded;// && load_cfg();

    if (!m_is_loaded) {
        //m_settings->gamepad = 0;
        if (!image_loaded) {
            m_settings->cx = 100; /* Default size */
            m_settings->cy = 100;
        }
    }

    return m_is_loaded;
}

void overlay::unload()
{
    unload_texture();
    m_settings->cx = 100;
    m_settings->cy = 100;
}


int pos = 40;
bool overlay::load_texture()
{
    //if (!m_settings || m_settings->image_file.empty())
    //    return false;

    auto flag = true;

    if (m_image == nullptr) {
        m_image = new gs_image_file_t();
    }
    else {
        ;// return true;
    }
    pos += 1;
    if (pos > 1000)pos = 40;
    QImage pxMap(1920, 1080, QImage::Format_ARGB32_Premultiplied);
    QPainter p;
    p.begin(&pxMap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setCompositionMode(QPainter::CompositionMode(3));

    QColor colorf = QColor::fromRgba(qRgba(255, 0, 0, 255));
    QColor colorn = QColor::fromRgba(qRgba(255, 0, 0, 0));//transparent

    p.setBrush(colorn);
    p.setPen(colorn); 
	p.drawRect(0, 0, 1920, 1080);

    p.setPen(QPen(colorf,3));
	p.drawLine(pos - 30, 400, pos + 30,400);
	p.drawLine(pos, 400-30, pos, 400+30);

    p.end();
    gs_image_file_init_test1(m_image,(char*) &pxMap);// "d:/test.png");// m_settings->image_file.c_str());

    obs_enter_graphics();
    gs_image_file_init_texture(m_image);
    obs_leave_graphics();

    if (!m_image->loaded) {
        bwarn("Error: failed to load texture %s", m_settings->image_file.c_str());
        flag = false;
    } else {
        m_settings->cx = m_image->cx;
        m_settings->cy = m_image->cy;
    }
    //tmptexture = gs_texture_create_gdi(1920, 1080);
    return flag;
}

void overlay::unload_texture() const
{
    obs_enter_graphics();
    gs_image_file_free_test1(m_image);
    obs_leave_graphics();
}

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

#pragma once

#include <QStringList>

class input_filter {
    QStringList m_filters;
    bool m_regex = false;
    bool m_whitelist = false;

public:
    ~input_filter();

    void read_from_config();

    void write_to_config();

    void add_filter(const char *filter);

    void remove_filter(int index);

    void set_regex(bool enabled);

    void set_whitelist(bool wl);

    bool input_blocked();

    QStringList &filters();
};

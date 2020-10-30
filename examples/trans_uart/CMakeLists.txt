#
#
#  GattLib - GATT Library -- app "trans_uart"  Transparent_UART for RN4871
#
#  Copyright (C) 2017  Olivier Martin <olivier@labapart.org>
#  Copyright (C) 2020  Robin Callender <robin@callender-consulting.com>
#
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

cmake_minimum_required(VERSION 2.6)

find_package(PkgConfig REQUIRED)

pkg_search_module(GATTLIB REQUIRED gattlib)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_search_module(PCRE REQUIRED libpcre)

include_directories(${GLIB_INCLUDE_DIRS})
set(trans_uart_SRCS trans_uart.c)

add_executable(trans_uart ${trans_uart_SRCS})
target_link_libraries(trans_uart ${GATTLIB_LIBRARIES} ${GATTLIB_LDFLAGS} ${GLIB_LDFLAGS} ${PCRE_LIBRARIES} pthread)

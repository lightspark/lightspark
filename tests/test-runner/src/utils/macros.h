/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2024  mr b0nk 500 (b0nk@b0nk.xyz)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef UTILS_MACROS_H
#define UTILS_MACROS_H 1

#define EMPTY(...)
#define IDENTITY(...) __VA_ARGS__
#define IDENTITY2(...) __VA_ARGS__

#define NAME_COMMA(name, ...) name,
#define MAKE_VARIANT(name, ...) (,) name IDENTITY
#define ENUM_STRUCT(name, x) using name = std::variant<IDENTITY2(EMPTY x(MAKE_VARIANT)())>

#endif /* UTILS_MACROS_H */

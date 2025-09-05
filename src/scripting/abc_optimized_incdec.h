/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2025  Ludger Krämer <dbluelle@onlinehome.de>

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

#ifndef SCRIPTING_ABC_OPTIMIZED_INCDEC_H
#define SCRIPTING_ABC_OPTIMIZED_INCDEC_H 1

namespace lightspark
{
struct call_context;
void abc_increment_constant(call_context* context);
void abc_increment_local(call_context* context);
void abc_increment_constant_localresult(call_context* context);
void abc_increment_local_localresult(call_context* context);

void abc_decrement_constant(call_context* context);
void abc_decrement_local(call_context* context);
void abc_decrement_constant_localresult(call_context* context);
void abc_decrement_local_localresult(call_context* context);

void abc_increment_i_local(call_context* context);
void abc_increment_i_local_localresult(call_context* context);
void abc_increment_i_local_setslotnocoerce(call_context* context);
void abc_decrement_i_local(call_context* context);
void abc_decrement_i_local_localresult(call_context* context);
void abc_decrement_i_local_setslotnocoerce(call_context* context);

void abc_inclocal_i_optimized(call_context* context);
void abc_inclocal_i_postfix(call_context* context);
void abc_declocal_i_optimized(call_context* context);
void abc_declocal_i_postfix(call_context* context);

}

#endif /* SCRIPTING_ABC_OPTIMIZED_INCDEC_H */

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

#ifndef SCRIPTING_ABC_OPTIMIZED_CALLPROPERTY_H
#define SCRIPTING_ABC_OPTIMIZED_CALLPROPERTY_H 1

namespace lightspark
{
struct call_context;
void abc_callpropertyStaticName(call_context* context);
void abc_callpropertyStaticName_localresult(call_context* context);
void abc_callpropertyStaticNameCached(call_context* context);
void abc_callpropertyStaticNameCached_localResult(call_context* context);
void abc_callpropertyStaticNameCached_constant(call_context* context);
void abc_callpropertyStaticNameCached_local(call_context* context);
void abc_callpropertyStaticNameCached_constant_localResult(call_context* context);
void abc_callpropertyStaticNameCached_local_localResult(call_context* context);
void abc_callpropertyStaticName_constant_constant(call_context* context);
void abc_callpropertyStaticName_local_constant(call_context* context);
void abc_callpropertyStaticName_constant_local(call_context* context);
void abc_callpropertyStaticName_local_local(call_context* context);
void abc_callpropertyStaticName_constant_constant_localresult(call_context* context);
void abc_callpropertyStaticName_local_constant_localresult(call_context* context);
void abc_callpropertyStaticName_constant_local_localresult(call_context* context);
void abc_callpropertyStaticName_local_local_localresult(call_context* context);
void abc_callpropertyStaticName_constant(call_context* context);
void abc_callpropertyStaticName_local(call_context* context);
void abc_callpropertyStaticName_constant_localresult(call_context* context);
void abc_callpropertyStaticName_local_localresult(call_context* context);

void abc_callpropvoidStaticName(call_context* context);
void abc_callpropvoidStaticNameCached(call_context* context);
void abc_callpropvoidStaticNameCached_constant(call_context* context);
void abc_callpropvoidStaticNameCached_local(call_context* context);
void abc_callpropvoidStaticName_constant_constant(call_context* context);
void abc_callpropvoidStaticName_local_constant(call_context* context);
void abc_callpropvoidStaticName_constant_local(call_context* context);
void abc_callpropvoidStaticName_local_local(call_context* context);
void abc_callpropvoidStaticName_constant(call_context* context);
void abc_callpropvoidStaticName_local(call_context* context);

void abc_callpropvoidSlotVarCached_constant(call_context* context);
void abc_callpropvoidSlotVarCached_local(call_context* context);
void abc_callpropvoidSlotVar_constant_constant(call_context* context);
void abc_callpropvoidSlotVar_local_constant(call_context* context);
void abc_callpropvoidSlotVar_constant_local(call_context* context);
void abc_callpropvoidSlotVar_local_local(call_context* context);
void abc_callpropvoidSlotVar_constant(call_context* context);
void abc_callpropvoidSlotVar_local(call_context* context);

void abc_callpropertySlotVar_constant_constant(call_context* context);
void abc_callpropertySlotVar_local_constant(call_context* context);
void abc_callpropertySlotVar_constant_local(call_context* context);
void abc_callpropertySlotVar_local_local(call_context* context);
void abc_callpropertySlotVar_constant_constant_localresult(call_context* context);
void abc_callpropertySlotVar_local_constant_localresult(call_context* context);
void abc_callpropertySlotVar_constant_local_localresult(call_context* context);
void abc_callpropertySlotVar_local_local_localresult(call_context* context);
void abc_callpropertySlotVar_constant(call_context* context);
void abc_callpropertySlotVar_local(call_context* context);
void abc_callpropertySlotVar_constant_localresult(call_context* context);
void abc_callpropertySlotVar_local_localresult(call_context* context);
void abc_callpropertySlotVarCached_constant(call_context* context);
void abc_callpropertySlotVarCached_local(call_context* context);
void abc_callpropertySlotVarCached_constant_localResult(call_context* context);
void abc_callpropertySlotVarCached_local_localResult(call_context* context);

void abc_callpropvoidBorrowedSlotCached_constant(call_context* context);
void abc_callpropvoidBorrowedSlotCached_local(call_context* context);
void abc_callpropvoidBorrowedSlot_constant(call_context* context);
void abc_callpropvoidBorrowedSlot_local(call_context* context);
void abc_callpropvoidBorrowedSlot_constant_constant(call_context* context);
void abc_callpropvoidBorrowedSlot_local_constant(call_context* context);
void abc_callpropvoidBorrowedSlot_constant_local(call_context* context);
void abc_callpropvoidBorrowedSlot_local_local(call_context* context);

void abc_callpropertyBorrowedSlot_constant(call_context* context);
void abc_callpropertyBorrowedSlot_local(call_context* context);
void abc_callpropertyBorrowedSlot_constant_localresult(call_context* context);
void abc_callpropertyBorrowedSlot_local_localresult(call_context* context);
void abc_callpropertyBorrowedSlot_constant_constant(call_context* context);
void abc_callpropertyBorrowedSlot_local_constant(call_context* context);
void abc_callpropertyBorrowedSlot_constant_local(call_context* context);
void abc_callpropertyBorrowedSlot_local_local(call_context* context);
void abc_callpropertyBorrowedSlot_constant_constant_localresult(call_context* context);
void abc_callpropertyBorrowedSlot_local_constant_localresult(call_context* context);
void abc_callpropertyBorrowedSlot_constant_local_localresult(call_context* context);
void abc_callpropertyBorrowedSlot_local_local_localresult(call_context* context);
void abc_callpropertyBorrowedSlotCached_constant(call_context* context);
void abc_callpropertyBorrowedSlotCached_local(call_context* context);
void abc_callpropertyBorrowedSlotCached_constant_localResult(call_context* context);
void abc_callpropertyBorrowedSlotCached_local_localResult(call_context* context);

}

#endif /* SCRIPTING_ABC_OPTIMIZED_CALLPROPERTY_H */

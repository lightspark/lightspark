/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/flash/globalization/collator.h"
#include "scripting/flash/globalization/datetimeformatter.h"
#include "scripting/flash/globalization/datetimestyle.h"
#include "scripting/flash/globalization/collator.h"
#include "scripting/flash/globalization/collatormode.h"
#include "scripting/flash/globalization/datetimenamecontext.h"
#include "scripting/flash/globalization/datetimenamestyle.h"
#include "scripting/flash/globalization/nationaldigitstype.h"
#include "scripting/flash/globalization/lastoperationstatus.h"
#include "scripting/flash/globalization/localeid.h"
#include "scripting/flash/globalization/currencyformatter.h"
#include "scripting/flash/globalization/currencyparseresult.h"
#include "scripting/flash/globalization/numberformatter.h"
#include "scripting/flash/globalization/numberparseresult.h"
#include "scripting/flash/globalization/stringtools.h"

#include "scripting/class.h"
#include "scripting/abc.h"
using namespace lightspark;


void ABCVm::registerClassesFlashGlobalization(Global* builtin)
{
	builtin->registerBuiltin("Collator","flash.globalization",Class<Collator>::getRef(m_sys));
	builtin->registerBuiltin("StringTools","flash.globalization",Class<StringTools>::getRef(m_sys));
	builtin->registerBuiltin("DateTimeFormatter","flash.globalization",Class<DateTimeFormatter>::getRef(m_sys));
	builtin->registerBuiltin("DateTimeStyle","flash.globalization",Class<DateTimeStyle>::getRef(m_sys));
	builtin->registerBuiltin("LastOperationStatus","flash.globalization",Class<LastOperationStatus>::getRef(m_sys));
	builtin->registerBuiltin("CurrencyFormatter","flash.globalization",Class<CurrencyFormatter>::getRef(m_sys));
	builtin->registerBuiltin("CurrencyParseResult","flash.globalization",Class<CurrencyParseResult>::getRef(m_sys));
	builtin->registerBuiltin("NumberFormatter","flash.globalization",Class<NumberFormatter>::getRef(m_sys));
	builtin->registerBuiltin("NumberParseResult","flash.globalization",Class<NumberParseResult>::getRef(m_sys));
	builtin->registerBuiltin("Collator","flash.globalization",Class<Collator>::getRef(m_sys));
	builtin->registerBuiltin("CollatorMode","flash.globalization",Class<CollatorMode>::getRef(m_sys));
	builtin->registerBuiltin("DateTimeNameContext","flash.globalization",Class<DateTimeNameContext>::getRef(m_sys));
	builtin->registerBuiltin("DateTimeNameStyle","flash.globalization",Class<DateTimeNameStyle>::getRef(m_sys));
	builtin->registerBuiltin("NationalDigitsType","flash.globalization",Class<NationalDigitsType>::getRef(m_sys));
	builtin->registerBuiltin("LocaleID","flash.globalization",Class<LocaleID>::getRef(m_sys));
}

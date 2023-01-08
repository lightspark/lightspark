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

#include <istream>
#include "parsing/tags.h"
#include "logger.h"
#include "compat.h"

using namespace std;
using namespace lightspark;

void lightspark::ignore(istream& i, int count);

ProtectTag::ProtectTag(RECORDHEADER h, istream& in):ControlTag(h)
{
	LOG(LOG_NOT_IMPLEMENTED,"Protect Tag");
	skip(in);
}

DefineFontNameTag::DefineFontNameTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_NOT_IMPLEMENTED,"DefineFontNameTag Tag");
	skip(in);
}

DefineFontAlignZonesTag::DefineFontAlignZonesTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_NOT_IMPLEMENTED,"DefineFontAlignZonesTag Tag");
	skip(in);
}

CSMTextSettingsTag::CSMTextSettingsTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_NOT_IMPLEMENTED,"CSMTextSettingsTag");
	skip(in);
}

UnimplementedTag::UnimplementedTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_NOT_IMPLEMENTED,"Unimplemented Tag " << h.getTagType());
	skip(in);
}

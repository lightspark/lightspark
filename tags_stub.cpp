/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "tags.h"
#include "logger.h"
#include "swf.h"

using namespace std;
using namespace lightspark;

extern TLSDATA ParseThread* pt;
void lightspark::ignore(istream& i, int count);

ProtectTag::ProtectTag(RECORDHEADER h, istream& in):ControlTag(h)
{
	LOG(LOG_NOT_IMPLEMENTED,"Protect Tag");
	skip(in);
}

DefineFontInfoTag::DefineFontInfoTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_NOT_IMPLEMENTED,"DefineFontInfo Tag");
	skip(in);
}

StartSoundTag::StartSoundTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_NOT_IMPLEMENTED,"StartSound Tag");
	skip(in);
}

SoundStreamHeadTag::SoundStreamHeadTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_NOT_IMPLEMENTED,"SoundStreamHead Tag");
	skip(in);
}

SoundStreamHead2Tag::SoundStreamHead2Tag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_NOT_IMPLEMENTED,"SoundStreamHead2 Tag");
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

DefineBitsJPEG2Tag::DefineBitsJPEG2Tag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_NOT_IMPLEMENTED,"DefineBitsJPEG2Tag Tag");
	skip(in);
}

DefineScalingGridTag::DefineScalingGridTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	in >> CharacterId >> Splitter;
	LOG(LOG_NOT_IMPLEMENTED,"DefineScalingGridTag Tag on ID " << CharacterId);
}

SoundStreamBlockTag::SoundStreamBlockTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_NOT_IMPLEMENTED,"SoundStreamBlockTag");
	skip(in);
}

DefineSceneAndFrameLabelDataTag::DefineSceneAndFrameLabelDataTag(RECORDHEADER h, std::istream& in):Tag(h)
{
	LOG(LOG_NOT_IMPLEMENTED,"DefineSceneAndFrameLabelDataTag");
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

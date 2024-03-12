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

#ifndef INTERFACES_BACKENDS_GRAPHICS_H
#define INTERFACES_BACKENDS_GRAPHICS_H 1

#include "forwards/swftypes.h"
#include "forwards/backends/graphics.h"
#include "forwards/scripting/flash/display/DisplayObject.h"
#include "smartrefs.h"
#include "swftypes.h"
#include <vector>
#include <cairo.h>

namespace lightspark
{

class ITextureUploadable
{
private:
	bool queued;
protected:
	~ITextureUploadable(){}
public:
	ITextureUploadable():queued(false) {}
	virtual void sizeNeeded(uint32_t& w, uint32_t& h) const=0;
	virtual void contentScale(number_t& x, number_t& y) const {x = 1; y = 1;}
	// Texture topleft from Shape origin
	virtual void contentOffset(number_t& x, number_t& y) const {x = 0; y = 0;}
	/*
		Upload data to memory mapped to the graphics card (note: size is guaranteed to be enough
	*/
	virtual uint8_t* upload(bool refresh)=0;
	virtual TextureChunk& getTexture()=0;
	/*
		Signal the completion of the upload to the texture
		NOTE: fence may be called on shutdown even if the upload has not happen, so be ready for this event
	*/
	virtual void uploadFence()
	{
		queued=false;
	}
	void setQueued() {queued=true;}
	bool getQueued() const { return queued;}
};

};
#endif /* INTERFACES_BACKENDS_GRAPHICS_H */

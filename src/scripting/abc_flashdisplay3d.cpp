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

#include "scripting/flash/display3d/flashdisplay3d.h"
#include "scripting/flash/display3d/flashdisplay3dtextures.h"

#include "scripting/class.h"
#include "scripting/abc.h"
using namespace lightspark;

void ABCVm::registerClassesFlashDisplay3D(Global* builtin)
{
	builtin->registerBuiltin("Context3D","flash.display3D",Class<Context3D>::getRef(m_sys));
	builtin->registerBuiltin("Context3DBlendFactor","flash.display3D",Class<Context3DBlendFactor>::getRef(m_sys));
	builtin->registerBuiltin("Context3DBufferUsage","flash.display3D",Class<Context3DBufferUsage>::getRef(m_sys));
	builtin->registerBuiltin("Context3DClearMask","flash.display3D",Class<Context3DClearMask>::getRef(m_sys));
	builtin->registerBuiltin("Context3DCompareMode","flash.display3D",Class<Context3DCompareMode>::getRef(m_sys));
	builtin->registerBuiltin("Context3DMipFilter","flash.display3D",Class<Context3DMipFilter>::getRef(m_sys));
	builtin->registerBuiltin("Context3DProfile","flash.display3D",Class<Context3DProfile>::getRef(m_sys));
	builtin->registerBuiltin("Context3DProgramType","flash.display3D",Class<Context3DProgramType>::getRef(m_sys));
	builtin->registerBuiltin("Context3DRenderMode","flash.display3D",Class<Context3DRenderMode>::getRef(m_sys));
	builtin->registerBuiltin("Context3DStencilAction","flash.display3D",Class<Context3DStencilAction>::getRef(m_sys));
	builtin->registerBuiltin("Context3DTextureFilter","flash.display3D",Class<Context3DTextureFilter>::getRef(m_sys));
	builtin->registerBuiltin("Context3DTextureFormat","flash.display3D",Class<Context3DTextureFormat>::getRef(m_sys));
	builtin->registerBuiltin("Context3DTriangleFace","flash.display3D",Class<Context3DTriangleFace>::getRef(m_sys));
	builtin->registerBuiltin("Context3DVertexBufferFormat","flash.display3D",Class<Context3DVertexBufferFormat>::getRef(m_sys));
	builtin->registerBuiltin("Context3DWrapMode","flash.display3D",Class<Context3DWrapMode>::getRef(m_sys));
	builtin->registerBuiltin("IndexBuffer3D","flash.display3D",Class<IndexBuffer3D>::getRef(m_sys));
	builtin->registerBuiltin("Program3D","flash.display3D",Class<Program3D>::getRef(m_sys));
	builtin->registerBuiltin("VertexBuffer3D","flash.display3D",Class<VertexBuffer3D>::getRef(m_sys));

	builtin->registerBuiltin("TextureBase","flash.display3D.textures",Class<TextureBase>::getRef(m_sys));
	builtin->registerBuiltin("Texture","flash.display3D.textures",Class<Texture>::getRef(m_sys));
	builtin->registerBuiltin("CubeTexture","flash.display3D.textures",Class<CubeTexture>::getRef(m_sys));
	builtin->registerBuiltin("RectangleTexture","flash.display3D.textures",Class<RectangleTexture>::getRef(m_sys));
	builtin->registerBuiltin("VideoTexture","flash.display3D.textures",Class<VideoTexture>::getRef(m_sys));
}

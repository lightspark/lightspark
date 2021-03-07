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

#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/geom/orientation3d.h"

#include "scripting/class.h"
#include "scripting/abc.h"
using namespace lightspark;

void ABCVm::registerClassesFlashGeom(Global* builtin)
{
	builtin->registerBuiltin("ColorTransform","flash.geom",Class<ColorTransform>::getRef(m_sys));
	builtin->registerBuiltin("Rectangle","flash.geom",Class<Rectangle>::getRef(m_sys));
	builtin->registerBuiltin("Matrix","flash.geom",Class<Matrix>::getRef(m_sys));
	builtin->registerBuiltin("Transform","flash.geom",Class<Transform>::getRef(m_sys));
	builtin->registerBuiltin("Point","flash.geom",Class<Point>::getRef(m_sys));
	builtin->registerBuiltin("Vector3D","flash.geom",Class<Vector3D>::getRef(m_sys));
	builtin->registerBuiltin("Matrix3D","flash.geom",Class<Matrix3D>::getRef(m_sys));
	builtin->registerBuiltin("Orientation3D","flash.geom",Class<Orientation3D>::getRef(m_sys));
	builtin->registerBuiltin("PerspectiveProjection","flash.geom",Class<PerspectiveProjection>::getRef(m_sys));
}

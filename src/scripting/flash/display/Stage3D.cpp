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

#include "scripting/flash/display/Stage3D.h"
#include "scripting/flash/display3d/flashdisplay3d.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/abc.h"
#include "backends/rendering.h"
#include "platforms/engineutils.h"


using namespace std;
using namespace lightspark;


bool Stage3D::renderImpl(RenderContext &ctxt) const
{
	if (context3D.isNull())
		return false;
	
	if (visible)
	{
		context3D->renderImpl(ctxt);
		// render backbuffer to stage
		EngineData* engineData = getSystemState()->getEngineData();
		engineData->exec_glActiveTexture_GL_TEXTURE0(SAMPLEPOSITION::SAMPLEPOS_STANDARD);
		engineData->exec_glBlendFunc(BLEND_ONE,BLEND_ONE_MINUS_SRC_ALPHA);
		engineData->exec_glUseProgram(((RenderThread&)ctxt).gpu_program);
		engineData->exec_glViewport(0,0,getSystemState()->getRenderThread()->windowWidth,getSystemState()->getRenderThread()->windowHeight);
		
		Vector2f offset = getSystemState()->getRenderThread()->getOffset();
		Vector2f scale = getSystemState()->getRenderThread()->getScale();
		((GLRenderContext&)ctxt).lsglLoadIdentity();
		((GLRenderContext&)ctxt).lsglTranslatef(x-offset.x,y-offset.y,0);
		((GLRenderContext&)ctxt).setMatrixUniform(GLRenderContext::LSGL_MODELVIEW);
		getSystemState()->getRenderThread()->renderTextureToFrameBuffer(context3D->backframebufferIDcurrent,context3D->backBufferWidth*scale.x,context3D->backBufferHeight*scale.y,nullptr,nullptr,true,true,true,true);
	}
	return true;
}

Stage3D::Stage3D(ASWorker* wrk, Class_base* c):EventDispatcher(wrk,c),x(0),y(0),visible(true)
{
	subtype = SUBTYPE_STAGE3D;
}

bool Stage3D::destruct()
{
	if (context3D)
		context3D->removeStoredMember();
	context3D.fakeRelease();
	return EventDispatcher::destruct();
}

void Stage3D::prepareShutdown()
{
	if (this->preparedforshutdown)
		return;
	EventDispatcher::prepareShutdown();
	if (context3D)
		context3D->prepareShutdown();
}

bool Stage3D::countCylicMemberReferences(garbagecollectorstate &gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = EventDispatcher::countCylicMemberReferences(gcstate);
	if (context3D)
		ret = context3D->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

void Stage3D::sinit(Class_base *c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("requestContext3D","",c->getSystemState()->getBuiltinFunction(requestContext3D),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("requestContext3DMatchingProfiles","",c->getSystemState()->getBuiltinFunction(requestContext3DMatchingProfiles),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,x,Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,y,Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,visible,Boolean);
	REGISTER_GETTER_RESULTTYPE(c,context3D,Context3D);
}
ASFUNCTIONBODY_GETTER_SETTER(Stage3D,x)
ASFUNCTIONBODY_GETTER_SETTER(Stage3D,y)
ASFUNCTIONBODY_GETTER_SETTER(Stage3D,visible)
ASFUNCTIONBODY_GETTER(Stage3D,context3D)

ASFUNCTIONBODY_ATOM(Stage3D,_constructor)
{
	//Stage3D* th=asAtomHandler::as<Stage3D>(obj);
	EventDispatcher::_constructor(ret,wrk,obj,nullptr,0);
}
ASFUNCTIONBODY_ATOM(Stage3D,requestContext3D)
{
	Stage3D* th=asAtomHandler::as<Stage3D>(obj);
	tiny_string context3DRenderMode;
	tiny_string profile;
	ARG_CHECK(ARG_UNPACK(context3DRenderMode,"auto")(profile,"baseline"));
	
	th->context3D = _MR(Class<Context3D>::getInstanceS(wrk));
	th->context3D->addStoredMember();
	th->context3D->init(th);
}
ASFUNCTIONBODY_ATOM(Stage3D,requestContext3DMatchingProfiles)
{
	//Stage3D* th=asAtomHandler::as<Stage3D>(obj);
	_NR<Vector> profiles;
	ARG_CHECK(ARG_UNPACK(profiles));
	LOG(LOG_NOT_IMPLEMENTED,"Stage3D.requestContext3DMatchingProfiles does nothing");
}

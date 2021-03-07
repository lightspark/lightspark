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

#include "scripting/flash/text/csmsettings.h"
#include "scripting/flash/text/flashtext.h"
#include "scripting/flash/text/flashtextengine.h"
#include "scripting/flash/text/textrenderer.h"

#include "scripting/class.h"
#include "scripting/abc.h"
using namespace lightspark;

void ABCVm::registerClassesFlashText(Global* builtin)
{
	builtin->registerBuiltin("AntiAliasType","flash.text",Class<AntiAliasType>::getRef(m_sys));
	builtin->registerBuiltin("CSMSettings","flash.text",Class<CSMSettings>::getRef(m_sys));
	builtin->registerBuiltin("Font","flash.text",Class<ASFont>::getRef(m_sys));
	builtin->registerBuiltin("FontStyle","flash.text",Class<FontStyle>::getRef(m_sys));
	builtin->registerBuiltin("FontType","flash.text",Class<FontType>::getRef(m_sys));
	builtin->registerBuiltin("GridFitType","flash.text",Class<GridFitType>::getRef(m_sys));
	builtin->registerBuiltin("StyleSheet","flash.text",Class<StyleSheet>::getRef(m_sys));
	builtin->registerBuiltin("TextColorType","flash.text",Class<TextColorType>::getRef(m_sys));
	builtin->registerBuiltin("TextDisplayMode","flash.text",Class<TextDisplayMode>::getRef(m_sys));
	builtin->registerBuiltin("TextField","flash.text",Class<TextField>::getRef(m_sys));
	builtin->registerBuiltin("TextFieldType","flash.text",Class<TextFieldType>::getRef(m_sys));
	builtin->registerBuiltin("TextFieldAutoSize","flash.text",Class<TextFieldAutoSize>::getRef(m_sys));
	builtin->registerBuiltin("TextFormat","flash.text",Class<TextFormat>::getRef(m_sys));
	builtin->registerBuiltin("TextFormatAlign","flash.text",Class<TextFormatAlign>::getRef(m_sys));
	builtin->registerBuiltin("TextLineMetrics","flash.text",Class<TextLineMetrics>::getRef(m_sys));
	builtin->registerBuiltin("TextInteractionMode","flash.text",Class<TextInteractionMode>::getRef(m_sys));
	builtin->registerBuiltin("TextRenderer","flash.text",Class<TextRenderer>::getRef(m_sys));
	builtin->registerBuiltin("StaticText","flash.text",Class<StaticText>::getRef(m_sys));

	builtin->registerBuiltin("BreakOpportunity","flash.text.engine",Class<BreakOpportunity>::getRef(m_sys));
	builtin->registerBuiltin("CFFHinting","flash.text.engine",Class<CFFHinting>::getRef(m_sys));
	builtin->registerBuiltin("ContentElement","flash.text.engine",Class<ContentElement>::getRef(m_sys));
	builtin->registerBuiltin("DigitCase","flash.text.engine",Class<DigitCase>::getRef(m_sys));
	builtin->registerBuiltin("DigitWidth","flash.text.engine",Class<DigitWidth>::getRef(m_sys));
	builtin->registerBuiltin("EastAsianJustifier","flash.text.engine",Class<EastAsianJustifier>::getRef(m_sys));
	builtin->registerBuiltin("ElementFormat","flash.text.engine",Class<ElementFormat>::getRef(m_sys));
	builtin->registerBuiltin("FontDescription","flash.text.engine",Class<FontDescription>::getRef(m_sys));
	builtin->registerBuiltin("FontMetrics","flash.text.engine",Class<FontMetrics>::getRef(m_sys));
	builtin->registerBuiltin("FontLookup","flash.text.engine",Class<FontLookup>::getRef(m_sys));
	builtin->registerBuiltin("FontPosture","flash.text.engine",Class<FontPosture>::getRef(m_sys));
	builtin->registerBuiltin("FontWeight","flash.text.engine",Class<FontWeight>::getRef(m_sys));
	builtin->registerBuiltin("GroupElement","flash.text.engine",Class<GroupElement>::getRef(m_sys));
	builtin->registerBuiltin("JustificationStyle","flash.text.engine",Class<JustificationStyle>::getRef(m_sys));
	builtin->registerBuiltin("Kerning","flash.text.engine",Class<Kerning>::getRef(m_sys));
	builtin->registerBuiltin("LigatureLevel","flash.text.engine",Class<LigatureLevel>::getRef(m_sys));
	builtin->registerBuiltin("LineJustification","flash.text.engine",Class<LineJustification>::getRef(m_sys));
	builtin->registerBuiltin("RenderingMode","flash.text.engine",Class<RenderingMode>::getRef(m_sys));
	builtin->registerBuiltin("SpaceJustifier","flash.text.engine",Class<SpaceJustifier>::getRef(m_sys));
	builtin->registerBuiltin("TabAlignment","flash.text.engine",Class<TabAlignment>::getRef(m_sys));
	builtin->registerBuiltin("TabStop","flash.text.engine",Class<TabStop>::getRef(m_sys));
	builtin->registerBuiltin("TextBaseline","flash.text.engine",Class<TextBaseline>::getRef(m_sys));
	builtin->registerBuiltin("TextBlock","flash.text.engine",Class<TextBlock>::getRef(m_sys));
	builtin->registerBuiltin("TextElement","flash.text.engine",Class<TextElement>::getRef(m_sys));
	builtin->registerBuiltin("TextLine","flash.text.engine",Class<TextLine>::getRef(m_sys));
	builtin->registerBuiltin("TextLineValidity","flash.text.engine",Class<TextLineValidity>::getRef(m_sys));
	builtin->registerBuiltin("TextRotation","flash.text.engine",Class<TextRotation>::getRef(m_sys));
	builtin->registerBuiltin("TextJustifier","flash.text.engine",Class<TextJustifier>::getRef(m_sys));
}

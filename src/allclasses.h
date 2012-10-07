/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

//DO NOT ADD INCLUDE GUARD HERE! This needs to be included twice in allclasses.cpp

REGISTER_CLASS_NAME(Array,"")
REGISTER_CLASS_NAME2(ASObject,"Object","")
REGISTER_CLASS_NAME2(ASQName,"QName","")
REGISTER_CLASS_NAME2(ASString, "String", "")
REGISTER_CLASS_NAME(Boolean,"")
REGISTER_CLASS_NAME(Date,"")
REGISTER_CLASS_NAME2(Global,"global","")
REGISTER_CLASS_NAME2(IFunction,"Function","")
REGISTER_CLASS_NAME2(Integer,"int","")
REGISTER_CLASS_NAME(Math,"")
REGISTER_CLASS_NAME(Namespace,"")
REGISTER_CLASS_NAME(Number,"")
REGISTER_CLASS_NAME(RegExp,"")
REGISTER_CLASS_NAME2(UInteger,"uint","")
REGISTER_CLASS_NAME2(ASError, "Error", "")
REGISTER_CLASS_NAME(SecurityError,"")
REGISTER_CLASS_NAME(ArgumentError,"")
REGISTER_CLASS_NAME(DefinitionError,"")
REGISTER_CLASS_NAME(EvalError,"")
REGISTER_CLASS_NAME(RangeError,"")
REGISTER_CLASS_NAME(ReferenceError,"")
REGISTER_CLASS_NAME(SyntaxError,"")
REGISTER_CLASS_NAME(TypeError,"")
REGISTER_CLASS_NAME(URIError,"")
REGISTER_CLASS_NAME(VerifyError,"")
REGISTER_CLASS_NAME(UninitializedError,"")
REGISTER_CLASS_NAME(XML,"")
REGISTER_CLASS_NAME(XMLList,"")

//Vectors
REGISTER_CLASS_NAME(Vector,"__AS3__.vec")

//Accessibility
REGISTER_CLASS_NAME(AccessibilityProperties,"flash.accessibility")
REGISTER_CLASS_NAME(AccessibilityImplementation,"flash.accessibility")

//Desktop (AIR)
REGISTER_CLASS_NAME(NativeApplication,"flash.desktop")

//Display
REGISTER_CLASS_NAME(AVM1Movie,"flash.display")
REGISTER_CLASS_NAME(Bitmap,"flash.display")
REGISTER_CLASS_NAME(BitmapData,"flash.display")
REGISTER_CLASS_NAME(BlendMode,"flash.display")
REGISTER_CLASS_NAME(DisplayObject,"flash.display")
REGISTER_CLASS_NAME(DisplayObjectContainer,"flash.display")
REGISTER_CLASS_NAME(FrameLabel,"flash.display")
REGISTER_CLASS_NAME(GradientType,"flash.display")
REGISTER_CLASS_NAME(Graphics,"flash.display")
REGISTER_CLASS_NAME(IBitmapDrawable,"flash.display")
REGISTER_CLASS_NAME(InteractiveObject,"flash.display")
REGISTER_CLASS_NAME(InterpolationMethod,"flash.display")
REGISTER_CLASS_NAME(LineScaleMode,"flash.display")
REGISTER_CLASS_NAME(Loader,"flash.display")
REGISTER_CLASS_NAME(LoaderInfo,"flash.display")
REGISTER_CLASS_NAME(MorphShape,"flash.display")
REGISTER_CLASS_NAME(MovieClip,"flash.display")
REGISTER_CLASS_NAME(Scene,"flash.display")
REGISTER_CLASS_NAME(Shader,"flash.display")
REGISTER_CLASS_NAME(Shape,"flash.display")
REGISTER_CLASS_NAME(SimpleButton,"flash.display")
REGISTER_CLASS_NAME(SpreadMethod,"flash.display")
REGISTER_CLASS_NAME(Sprite,"flash.display")
REGISTER_CLASS_NAME(Stage,"flash.display")
REGISTER_CLASS_NAME(StageAlign,"flash.display")
REGISTER_CLASS_NAME(StageDisplayState,"flash.display")
REGISTER_CLASS_NAME(StageScaleMode,"flash.display")
REGISTER_CLASS_NAME(StageQuality,"flash.display")

//Events
REGISTER_CLASS_NAME(AsyncErrorEvent,"flash.events")
REGISTER_CLASS_NAME(DRMErrorEvent,"flash.events")
REGISTER_CLASS_NAME(DRMStatusEvent,"flash.events")
REGISTER_CLASS_NAME(DataEvent,"flash.events")
REGISTER_CLASS_NAME(ErrorEvent,"flash.events")
REGISTER_CLASS_NAME(Event,"flash.events")
REGISTER_CLASS_NAME(EventDispatcher,"flash.events")
REGISTER_CLASS_NAME(EventPhase,"flash.events")
REGISTER_CLASS_NAME(FocusEvent,"flash.events")
REGISTER_CLASS_NAME(FullScreenEvent,"flash.events")
REGISTER_CLASS_NAME(HTTPStatusEvent,"flash.events")
REGISTER_CLASS_NAME(IEventDispatcher,"flash.events")
REGISTER_CLASS_NAME(IOErrorEvent,"flash.events")
REGISTER_CLASS_NAME(InvokeEvent,"flash.events")
REGISTER_CLASS_NAME(KeyboardEvent,"flash.events")
REGISTER_CLASS_NAME(MouseEvent,"flash.events")
REGISTER_CLASS_NAME(NetStatusEvent,"flash.events")
REGISTER_CLASS_NAME(ProgressEvent,"flash.events")
REGISTER_CLASS_NAME(SecurityErrorEvent,"flash.events")
REGISTER_CLASS_NAME(StageVideoEvent,"flash.events")
REGISTER_CLASS_NAME(StageVideoAvailabilityEvent,"flash.events")
REGISTER_CLASS_NAME(StatusEvent,"flash.events")
REGISTER_CLASS_NAME(TextEvent,"flash.events")
REGISTER_CLASS_NAME(TimerEvent,"flash.events")

//External interface (browser interaction)
REGISTER_CLASS_NAME(ExternalInterface,"flash.external")

//Filters
REGISTER_CLASS_NAME(BitmapFilter,"flash.filters")
REGISTER_CLASS_NAME(DropShadowFilter,"flash.filters")
REGISTER_CLASS_NAME(GlowFilter,"flash.filters")

//Geom
REGISTER_CLASS_NAME(ColorTransform,"flash.geom")
REGISTER_CLASS_NAME(Matrix,"flash.geom")
REGISTER_CLASS_NAME(Point,"flash.geom")
REGISTER_CLASS_NAME2(Rectangle,"Rectangle","flash.geom")
REGISTER_CLASS_NAME(Transform,"flash.geom")
REGISTER_CLASS_NAME(Vector3D,"flash.geom")

//Media
REGISTER_CLASS_NAME(Sound,"flash.media")
REGISTER_CLASS_NAME(SoundChannel,"flash.media")
REGISTER_CLASS_NAME(SoundLoaderContext,"flash.media")
REGISTER_CLASS_NAME(SoundTransform,"flash.media")
REGISTER_CLASS_NAME(StageVideo,"flash.media")
REGISTER_CLASS_NAME(StageVideoAvailability,"flash.media")
REGISTER_CLASS_NAME(Video,"flash.media")
REGISTER_CLASS_NAME(VideoStatus,"flash.media")

//Net
REGISTER_CLASS_NAME(NetConnection,"flash.net")
REGISTER_CLASS_NAME(NetStream,"flash.net")
REGISTER_CLASS_NAME(ObjectEncoding,"flash.net")
REGISTER_CLASS_NAME(Responder,"flash.net")
REGISTER_CLASS_NAME(SharedObject,"flash.net")
REGISTER_CLASS_NAME(URLLoader,"flash.net")
REGISTER_CLASS_NAME(URLLoaderDataFormat,"flash.net")
REGISTER_CLASS_NAME(URLRequest,"flash.net")
REGISTER_CLASS_NAME(URLRequestHeader,"flash.net")
REGISTER_CLASS_NAME(URLRequestMethod,"flash.net")
REGISTER_CLASS_NAME(URLStream,"flash.net")
REGISTER_CLASS_NAME(URLVariables,"flash.net")
REGISTER_CLASS_NAME(XMLSocket,"flash.net")

//Errors
REGISTER_CLASS_NAME(EOFError,"flash.errors")
REGISTER_CLASS_NAME(IOError,"flash.errors")
REGISTER_CLASS_NAME(IllegalOperationError,"flash.errors")
REGISTER_CLASS_NAME(InvalidSWFError,"flash.errors")
REGISTER_CLASS_NAME(MemoryError,"flash.errors")
REGISTER_CLASS_NAME(ScriptTimeoutError,"flash.errors")
REGISTER_CLASS_NAME(StackOverflowError,"flash.errors")

//Sensors
REGISTER_CLASS_NAME(Accelerometer,"flash.sensors")

//System
REGISTER_CLASS_NAME(ApplicationDomain,"flash.system")
REGISTER_CLASS_NAME(Capabilities,"flash.system")
REGISTER_CLASS_NAME(LoaderContext,"flash.system")
REGISTER_CLASS_NAME(Security,"flash.system")
REGISTER_CLASS_NAME(SecurityDomain,"flash.system")
REGISTER_CLASS_NAME(System,"flash.system")

//Text
REGISTER_CLASS_NAME2(ASFont,"Font","flash.text")
REGISTER_CLASS_NAME(AntiAliasType,"flash.text")
REGISTER_CLASS_NAME(FontStyle,"flash.text")
REGISTER_CLASS_NAME(FontType,"flash.text")
REGISTER_CLASS_NAME(GridFitType,"flash.text")
REGISTER_CLASS_NAME(TextColorType,"flash.text")
REGISTER_CLASS_NAME(TextDisplayMode,"flash.text")
REGISTER_CLASS_NAME(TextField,"flash.text")
REGISTER_CLASS_NAME(TextFieldAutoSize,"flash.text")
REGISTER_CLASS_NAME(TextFieldType,"flash.text")
REGISTER_CLASS_NAME(TextFormat,"flash.text")
REGISTER_CLASS_NAME(TextFormatAlign,"flash.text")
REGISTER_CLASS_NAME(StaticText,"flash.text")
REGISTER_CLASS_NAME(StyleSheet,"flash.text")

//Text engine
REGISTER_CLASS_NAME(ContentElement,"flash.text.engine")
REGISTER_CLASS_NAME(ElementFormat,"flash.text.engine")
REGISTER_CLASS_NAME(FontDescription,"flash.text.engine")
REGISTER_CLASS_NAME(FontWeight,"flash.text.engine")
REGISTER_CLASS_NAME(TextBlock,"flash.text.engine")
REGISTER_CLASS_NAME(TextElement,"flash.text.engine")
REGISTER_CLASS_NAME(TextLine,"flash.text.engine")

//Utils
REGISTER_CLASS_NAME(ByteArray,"flash.utils")
REGISTER_CLASS_NAME(Dictionary,"flash.utils")
REGISTER_CLASS_NAME(Endian,"flash.utils")
REGISTER_CLASS_NAME(IDataInput,"flash.utils")
REGISTER_CLASS_NAME(IDataOutput,"flash.utils")
REGISTER_CLASS_NAME(IExternalizable,"flash.utils")
REGISTER_CLASS_NAME(Proxy,"flash.utils")
REGISTER_CLASS_NAME(Timer,"flash.utils")

//XML
REGISTER_CLASS_NAME(XMLDocument,"flash.xml")
REGISTER_CLASS_NAME(XMLNode,"flash.xml")


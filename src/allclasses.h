/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
REGISTER_CLASS_NAME(JSON,"")
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
REGISTER_CLASS_NAME(Accessibility,"flash.accessibility")
REGISTER_CLASS_NAME(AccessibilityProperties,"flash.accessibility")
REGISTER_CLASS_NAME(AccessibilityImplementation,"flash.accessibility")

//Concurrent
REGISTER_CLASS_NAME(ASMutex,"flash.concurrent")
REGISTER_CLASS_NAME(ASCondition,"flash.concurrent")

//Desktop (AIR)
REGISTER_CLASS_NAME(NativeApplication,"flash.desktop")
REGISTER_CLASS_NAME(NativeDragManager,"flash.desktop")


//Display
REGISTER_CLASS_NAME(AVM1Movie,"flash.display")
REGISTER_CLASS_NAME(Bitmap,"flash.display")
REGISTER_CLASS_NAME(BitmapData,"flash.display")
REGISTER_CLASS_NAME(BitmapDataChannel,"flash.display")
REGISTER_CLASS_NAME(BlendMode,"flash.display")
REGISTER_CLASS_NAME(CapsStyle,"flash.display")
REGISTER_CLASS_NAME(DisplayObject,"flash.display")
REGISTER_CLASS_NAME(DisplayObjectContainer,"flash.display")
REGISTER_CLASS_NAME(FrameLabel,"flash.display")
REGISTER_CLASS_NAME(GradientType,"flash.display")
REGISTER_CLASS_NAME(Graphics,"flash.display")
REGISTER_CLASS_NAME(GraphicsBitmapFill,"flash.display")
REGISTER_CLASS_NAME(GraphicsEndFill,"flash.display")
REGISTER_CLASS_NAME(GraphicsGradientFill,"flash.display")
REGISTER_CLASS_NAME(GraphicsPath,"flash.display")
REGISTER_CLASS_NAME(GraphicsPathCommand,"flash.display")
REGISTER_CLASS_NAME(GraphicsPathWinding,"flash.display")
REGISTER_CLASS_NAME(GraphicsShaderFill,"flash.display")
REGISTER_CLASS_NAME(GraphicsSolidFill,"flash.display")
REGISTER_CLASS_NAME(GraphicsStroke,"flash.display")
REGISTER_CLASS_NAME(GraphicsTrianglePath,"flash.display")
REGISTER_CLASS_NAME(IGraphicsData,"flash.display")
REGISTER_CLASS_NAME(IGraphicsFill,"flash.display")
REGISTER_CLASS_NAME(IGraphicsPath,"flash.display")
REGISTER_CLASS_NAME(IGraphicsStroke,"flash.display")
REGISTER_CLASS_NAME(IBitmapDrawable,"flash.display")
REGISTER_CLASS_NAME(InteractiveObject,"flash.display")
REGISTER_CLASS_NAME(InterpolationMethod,"flash.display")
REGISTER_CLASS_NAME(JointStyle,"flash.display")
REGISTER_CLASS_NAME(LineScaleMode,"flash.display")
REGISTER_CLASS_NAME(Loader,"flash.display")
REGISTER_CLASS_NAME(LoaderInfo,"flash.display")
REGISTER_CLASS_NAME(MorphShape,"flash.display")
REGISTER_CLASS_NAME(MovieClip,"flash.display")
REGISTER_CLASS_NAME(PixelSnapping,"flash.display")
REGISTER_CLASS_NAME(Scene,"flash.display")
REGISTER_CLASS_NAME(Shader,"flash.display")
REGISTER_CLASS_NAME(Shape,"flash.display")
REGISTER_CLASS_NAME(SimpleButton,"flash.display")
REGISTER_CLASS_NAME(SpreadMethod,"flash.display")
REGISTER_CLASS_NAME(Sprite,"flash.display")
REGISTER_CLASS_NAME(Stage,"flash.display")
REGISTER_CLASS_NAME(Stage3D,"flash.display")
REGISTER_CLASS_NAME(StageAlign,"flash.display")
REGISTER_CLASS_NAME(StageDisplayState,"flash.display")
REGISTER_CLASS_NAME(StageScaleMode,"flash.display")
REGISTER_CLASS_NAME(StageQuality,"flash.display")

//Display3D
REGISTER_CLASS_NAME(Context3D,"flash.display3D")
REGISTER_CLASS_NAME(Context3DBlendFactor,"flash.display3D")
REGISTER_CLASS_NAME(Context3DBufferUsage,"flash.display3D")
REGISTER_CLASS_NAME(Context3DClearMask,"flash.display3D")
REGISTER_CLASS_NAME(Context3DCompareMode,"flash.display3D")
REGISTER_CLASS_NAME(Context3DMipFilter,"flash.display3D")
REGISTER_CLASS_NAME(Context3DProfile,"flash.display3D")
REGISTER_CLASS_NAME(Context3DProgramType,"flash.display3D")
REGISTER_CLASS_NAME(Context3DRenderMode,"flash.display3D")
REGISTER_CLASS_NAME(Context3DStencilAction,"flash.display3D")
REGISTER_CLASS_NAME(Context3DTextureFilter,"flash.display3D")
REGISTER_CLASS_NAME(Context3DTextureFormat,"flash.display3D")
REGISTER_CLASS_NAME(Context3DTriangleFace,"flash.display3D")
REGISTER_CLASS_NAME(Context3DVertexBufferFormat,"flash.display3D")
REGISTER_CLASS_NAME(Context3DWrapMode,"flash.display3D")
REGISTER_CLASS_NAME(IndexBuffer3D,"flash.display3D")
REGISTER_CLASS_NAME(Program3D,"flash.display3D")
REGISTER_CLASS_NAME(VertexBuffer3D,"flash.display3D")

//Display3D.textures
REGISTER_CLASS_NAME(TextureBase,"flash.display3D.textures")
REGISTER_CLASS_NAME(Texture,"flash.display3D.textures")
REGISTER_CLASS_NAME(CubeTexture,"flash.display3D.textures")
REGISTER_CLASS_NAME(RectangleTexture,"flash.display3D.textures")
REGISTER_CLASS_NAME(VideoTexture,"flash.display3D.textures")

//Events
REGISTER_CLASS_NAME(AsyncErrorEvent,"flash.events")
REGISTER_CLASS_NAME(ContextMenuEvent,"flash.events")
REGISTER_CLASS_NAME(DRMErrorEvent,"flash.events")
REGISTER_CLASS_NAME(DRMStatusEvent,"flash.events")
REGISTER_CLASS_NAME(DataEvent,"flash.events")
REGISTER_CLASS_NAME(ErrorEvent,"flash.events")
REGISTER_CLASS_NAME(Event,"flash.events")
REGISTER_CLASS_NAME(EventDispatcher,"flash.events")
REGISTER_CLASS_NAME(EventPhase,"flash.events")
REGISTER_CLASS_NAME(FocusEvent,"flash.events")
REGISTER_CLASS_NAME(FullScreenEvent,"flash.events")
REGISTER_CLASS_NAME(GestureEvent,"flash.events")
REGISTER_CLASS_NAME(HTTPStatusEvent,"flash.events")
REGISTER_CLASS_NAME(IEventDispatcher,"flash.events")
REGISTER_CLASS_NAME(IOErrorEvent,"flash.events")
REGISTER_CLASS_NAME(InvokeEvent,"flash.events")
REGISTER_CLASS_NAME(KeyboardEvent,"flash.events")
REGISTER_CLASS_NAME(MouseEvent,"flash.events")
REGISTER_CLASS_NAME(NativeDragEvent,"flash.events")
REGISTER_CLASS_NAME(NetStatusEvent,"flash.events")
REGISTER_CLASS_NAME(PressAndTapGestureEvent,"flash.events")
REGISTER_CLASS_NAME(ProgressEvent,"flash.events")
REGISTER_CLASS_NAME(SecurityErrorEvent,"flash.events")
REGISTER_CLASS_NAME(StageVideoEvent,"flash.events")
REGISTER_CLASS_NAME(StageVideoAvailabilityEvent,"flash.events")
REGISTER_CLASS_NAME(StatusEvent,"flash.events")
REGISTER_CLASS_NAME(TextEvent,"flash.events")
REGISTER_CLASS_NAME(TimerEvent,"flash.events")
REGISTER_CLASS_NAME(TouchEvent,"flash.events")
REGISTER_CLASS_NAME(TransformGestureEvent,"flash.events")
REGISTER_CLASS_NAME(UncaughtErrorEvent,"flash.events")
REGISTER_CLASS_NAME(UncaughtErrorEvents,"flash.events")
REGISTER_CLASS_NAME(VideoEvent,"flash.events")

//External interface (browser interaction)
REGISTER_CLASS_NAME(ExternalInterface,"flash.external")

//filesystem
REGISTER_CLASS_NAME2(ASFile,"File","flash.filesystem")
REGISTER_CLASS_NAME(FileStream,"flash.filesystem")

//Filters
REGISTER_CLASS_NAME(BitmapFilter,"flash.filters")
REGISTER_CLASS_NAME(BitmapFilterQuality,"flash.filters")
REGISTER_CLASS_NAME(DropShadowFilter,"flash.filters")
REGISTER_CLASS_NAME(GlowFilter,"flash.filters")
REGISTER_CLASS_NAME(GradientGlowFilter,"flash.filters")
REGISTER_CLASS_NAME(BevelFilter,"flash.filters")
REGISTER_CLASS_NAME(ColorMatrixFilter,"flash.filters")
REGISTER_CLASS_NAME(BlurFilter,"flash.filters")
REGISTER_CLASS_NAME(ConvolutionFilter,"flash.filters")
REGISTER_CLASS_NAME(DisplacementMapFilter,"flash.filters")
REGISTER_CLASS_NAME(DisplacementMapFilterMode,"flash.filters")
REGISTER_CLASS_NAME(GradientBevelFilter,"flash.filters")
REGISTER_CLASS_NAME(ShaderFilter,"flash.filters")


//Geom
REGISTER_CLASS_NAME(ColorTransform,"flash.geom")
REGISTER_CLASS_NAME(Matrix,"flash.geom")
REGISTER_CLASS_NAME(Point,"flash.geom")
REGISTER_CLASS_NAME2(Rectangle,"Rectangle","flash.geom")
REGISTER_CLASS_NAME(Transform,"flash.geom")
REGISTER_CLASS_NAME(Vector3D,"flash.geom")
REGISTER_CLASS_NAME(Matrix3D,"flash.geom")
REGISTER_CLASS_NAME(PerspectiveProjection,"flash.geom")

//Media
REGISTER_CLASS_NAME(Sound,"flash.media")
REGISTER_CLASS_NAME(SoundChannel,"flash.media")
REGISTER_CLASS_NAME(SoundLoaderContext,"flash.media")
REGISTER_CLASS_NAME(SoundMixer,"flash.media")
REGISTER_CLASS_NAME(SoundTransform,"flash.media")
REGISTER_CLASS_NAME(StageVideo,"flash.media")
REGISTER_CLASS_NAME(StageVideoAvailability,"flash.media")
REGISTER_CLASS_NAME(Video,"flash.media")
REGISTER_CLASS_NAME(VideoStatus,"flash.media")
REGISTER_CLASS_NAME(Microphone,"flash.media")
REGISTER_CLASS_NAME(Camera,"flash.media")
REGISTER_CLASS_NAME(VideoStreamSettings,"flash.media")
REGISTER_CLASS_NAME(H264VideoStreamSettings,"flash.media")


//Net
REGISTER_CLASS_NAME(FileFilter,"flash.net")
REGISTER_CLASS_NAME(FileReference,"flash.net")
REGISTER_CLASS_NAME(LocalConnection,"flash.net")
REGISTER_CLASS_NAME(NetConnection,"flash.net")
REGISTER_CLASS_NAME(NetGroup,"flash.net")
REGISTER_CLASS_NAME(NetStream,"flash.net")
REGISTER_CLASS_NAME(NetStreamAppendBytesAction,"flash.net")
REGISTER_CLASS_NAME(NetStreamInfo,"flash.net")
REGISTER_CLASS_NAME(NetStreamPlayTransitions,"flash.net")
REGISTER_CLASS_NAME(NetStreamPlayOptions,"flash.net")
REGISTER_CLASS_NAME(ObjectEncoding,"flash.net")
REGISTER_CLASS_NAME(Responder,"flash.net")
REGISTER_CLASS_NAME(SharedObject,"flash.net")
REGISTER_CLASS_NAME(SharedObjectFlushStatus,"flash.net")
REGISTER_CLASS_NAME(ASSocket,"flash.net")
REGISTER_CLASS_NAME(URLLoader,"flash.net")
REGISTER_CLASS_NAME(URLLoaderDataFormat,"flash.net")
REGISTER_CLASS_NAME(URLRequest,"flash.net")
REGISTER_CLASS_NAME(URLRequestHeader,"flash.net")
REGISTER_CLASS_NAME(URLRequestMethod,"flash.net")
REGISTER_CLASS_NAME(URLStream,"flash.net")
REGISTER_CLASS_NAME(URLVariables,"flash.net")
REGISTER_CLASS_NAME(XMLSocket,"flash.net")

REGISTER_CLASS_NAME(DRMManager,"flash.net.drm")

//Printing
REGISTER_CLASS_NAME(PrintJob,"flash.printing")
REGISTER_CLASS_NAME(PrintJobOptions,"flash.printing")
REGISTER_CLASS_NAME(PrintJobOrientation,"flash.printing")

//Errors
REGISTER_CLASS_NAME(EOFError,"flash.errors")
REGISTER_CLASS_NAME(IOError,"flash.errors")
REGISTER_CLASS_NAME(IllegalOperationError,"flash.errors")
REGISTER_CLASS_NAME(InvalidSWFError,"flash.errors")
REGISTER_CLASS_NAME(MemoryError,"flash.errors")
REGISTER_CLASS_NAME(ScriptTimeoutError,"flash.errors")
REGISTER_CLASS_NAME(StackOverflowError,"flash.errors")

//Sampler
REGISTER_CLASS_NAME(DeleteObjectSample,"flash.sampler")
REGISTER_CLASS_NAME(NewObjectSample,"flash.sampler")
REGISTER_CLASS_NAME(Sample,"flash.sampler")
REGISTER_CLASS_NAME(StackFrame,"flash.sampler")

//Sensors
REGISTER_CLASS_NAME(Accelerometer,"flash.sensors")

//System
REGISTER_CLASS_NAME(ApplicationDomain,"flash.system")
REGISTER_CLASS_NAME(Capabilities,"flash.system")
REGISTER_CLASS_NAME(LoaderContext,"flash.system")
REGISTER_CLASS_NAME(Security,"flash.system")
REGISTER_CLASS_NAME(SecurityDomain,"flash.system")
REGISTER_CLASS_NAME(System,"flash.system")
REGISTER_CLASS_NAME(ASWorker,"flash.system")
REGISTER_CLASS_NAME(ImageDecodingPolicy,"flash.system")
REGISTER_CLASS_NAME(IMEConversionMode,"flash.system")


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
REGISTER_CLASS_NAME(TextInteractionMode,"flash.text")
REGISTER_CLASS_NAME(TextLineMetrics,"flash.text")
REGISTER_CLASS_NAME(StaticText,"flash.text")
REGISTER_CLASS_NAME(StyleSheet,"flash.text")

//Text engine
REGISTER_CLASS_NAME(BreakOpportunity,"flash.text.engine")
REGISTER_CLASS_NAME(CFFHinting,"flash.text.engine")
REGISTER_CLASS_NAME(ContentElement,"flash.text.engine")
REGISTER_CLASS_NAME(DigitCase,"flash.text.engine")
REGISTER_CLASS_NAME(DigitWidth,"flash.text.engine")
REGISTER_CLASS_NAME(EastAsianJustifier,"flash.text.engine")
REGISTER_CLASS_NAME(ElementFormat,"flash.text.engine")
REGISTER_CLASS_NAME(FontDescription,"flash.text.engine")
REGISTER_CLASS_NAME(FontMetrics,"flash.text.engine")
REGISTER_CLASS_NAME(FontLookup,"flash.text.engine")
REGISTER_CLASS_NAME(FontPosture,"flash.text.engine")
REGISTER_CLASS_NAME(FontWeight,"flash.text.engine")
REGISTER_CLASS_NAME(GroupElement,"flash.text.engine")
REGISTER_CLASS_NAME(JustificationStyle,"flash.text.engine")
REGISTER_CLASS_NAME(Kerning,"flash.text.engine")
REGISTER_CLASS_NAME(LigatureLevel,"flash.text.engine")
REGISTER_CLASS_NAME(LineJustification,"flash.text.engine")
REGISTER_CLASS_NAME(RenderingMode,"flash.text.engine")
REGISTER_CLASS_NAME(SpaceJustifier,"flash.text.engine")
REGISTER_CLASS_NAME(TabAlignment,"flash.text.engine")
REGISTER_CLASS_NAME(TabStop,"flash.text.engine")
REGISTER_CLASS_NAME(TextBaseline,"flash.text.engine")
REGISTER_CLASS_NAME(TextBlock,"flash.text.engine")
REGISTER_CLASS_NAME(TextElement,"flash.text.engine")
REGISTER_CLASS_NAME(TextJustifier,"flash.text.engine")
REGISTER_CLASS_NAME(TextLine,"flash.text.engine")
REGISTER_CLASS_NAME(TextLineValidity,"flash.text.engine")
REGISTER_CLASS_NAME(TextRotation,"flash.text.engine")

//Utils
REGISTER_CLASS_NAME(ByteArray,"flash.utils")
REGISTER_CLASS_NAME(CompressionAlgorithm,"flash.utils")
REGISTER_CLASS_NAME(Dictionary,"flash.utils")
REGISTER_CLASS_NAME(Endian,"flash.utils")
REGISTER_CLASS_NAME(IDataInput,"flash.utils")
REGISTER_CLASS_NAME(IDataOutput,"flash.utils")
REGISTER_CLASS_NAME(IExternalizable,"flash.utils")
REGISTER_CLASS_NAME(Proxy,"flash.utils")
REGISTER_CLASS_NAME(Timer,"flash.utils")

//UI
REGISTER_CLASS_NAME(Keyboard,"flash.ui")
REGISTER_CLASS_NAME(KeyboardType,"flash.ui")
REGISTER_CLASS_NAME(KeyLocation,"flash.ui")
REGISTER_CLASS_NAME(Mouse,"flash.ui")
REGISTER_CLASS_NAME(MouseCursor,"flash.ui")
REGISTER_CLASS_NAME(MouseCursorData,"flash.ui")
REGISTER_CLASS_NAME(Multitouch,"flash.ui")
REGISTER_CLASS_NAME(MultitouchInputMode,"flash.ui")
REGISTER_CLASS_NAME(ContextMenu,"flash.ui")
REGISTER_CLASS_NAME(ContextMenuItem,"flash.ui")
REGISTER_CLASS_NAME(ContextMenuBuiltInItems,"flash.ui")

//XML
REGISTER_CLASS_NAME(XMLDocument,"flash.xml")
REGISTER_CLASS_NAME(XMLNode,"flash.xml")

//avmplus
REGISTER_CLASS_NAME(avmplusFile,"avmplus")
REGISTER_CLASS_NAME(avmplusSystem,"avmplus")
REGISTER_CLASS_NAME(avmplusDomain,"avmplus")



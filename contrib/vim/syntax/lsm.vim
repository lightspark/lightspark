" Vim syntax file for Lightspark Movie (LSM) files.
" Language: Lightspark Movie (LSM)
" Maintainer: mr b0nk 500
" Initial Revision: December 05 2024
" Latest Revision: December 19 2024

if exists("b:current_syntax")
	finish
endif

let s:cpo_save = &cpo
set cpo&vim

" Keywords.
syn keyword BlockKeyword header frame

" Short form event types.
syn match ShortEventType /M\([mbwudcD]\)*/
syn match ShortEventType /K\([udpc]\)*/
syn match ShortEventType /T\([ic]\)*/
syn match ShortEventType /W\([rmf]\)*/
syn keyword ShortEventType C

" Short form event sub-types.
syn keyword ShortMouseType m b w
syn keyword ShortKeyType u d p c
syn keyword ShortTextType i c
syn keyword ShortWindowType r m f
syn keyword ShortMouseButtonType u d c D
syn keyword ShortWindowFocusType k m
" Short form event keywords.
syn keyword ShortMouseButton l m r
syn keyword ShortKeyModifier n c s a S

" Event types.
syn match EventType /Mouse\(Move\|Button\|Wheel\|Up\|Down\|\(Double\)\=Click\)*/
syn match EventType /Key\(Up\|Down\|Press\|Control\)*/
syn match EventType /Text\(Input\|Clipboard\)*/
syn match EventType /Window\(Resize\|Move\|Focus\)*/
syn keyword EventType ContextMenu

" Event sub-types.
syn keyword MouseType Move Button Wheel
syn keyword KeyType Up Down Press Control
syn keyword TextType Input Clipboard
syn keyword WindowType Resize Move Focus
syn keyword MouseButtonType Up Down Click DoubleClick
syn keyword WindowFocusType keyboard mouse
" Event keywords.
syn keyword MouseButton Left Middle Right
syn keyword KeyModifier None Ctrl Shift Alt Super
" Keycodes.
syn keyword NonCharKeyCodes MouseLeft MouseRight MouseMiddle
syn keyword NonCharKeyCodes Backspace Tab Return Command Shift Ctrl Alt
syn keyword NonCharKeyCodes Pause CapsLock Numpad Escape Space PgUp PgDown
syn keyword NonCharKeyCodes End Home Left Up Right Down Insert Delete
syn keyword NonCharKeyCodes Numpad0 Numpad1 Numpad2 Numpad3 Numpad4
syn keyword NonCharKeyCodes Numpad5 Numpad6 Numpad7 Numpad8 Numpad9
syn keyword NonCharKeyCodes NumpadEnter NumpadMinus NumpadPeriod
syn keyword NonCharKeyCodes NumpadSlash F1 F2 F3 F4 F5 F6 F7 F8 F9 F10

syn keyword NonCharKeyCodes F11 F12 F13 F14 F15 F16 F17 F18 F19 F20 F21
syn keyword NonCharKeyCodes F22 F23 F24 NumLock ScrollLock
" Keycode aliases.
syn keyword NonCharKeyCodes LeftArrow UpArrow RightArrow DownArrow Del Ins

" Other keywords
syn keyword ScaleMode exactFit noBorder noScale showAll
syn keyword MovieEndMode stop play keepPlaying loop repeat repeatEvents loopEvents

" Special identifiers.

" Header identifiers.
syn keyword HeaderIdents player version name description author file md5
syn keyword HeaderIdents sha1 frames rerecords allowNoScale resolution
syn keyword HeaderIdents endMode seed timeStep scaling
" Header aliases.
syn keyword HeaderIdents desc info authors numFrames frameCount
syn keyword HeaderIdents numRerecords rerecordCount numSaveStates
syn keyword HeaderIdents saveStateCount res dimensions movieEndMode
syn keyword HeaderIdents onMovieEnd startingSeed rngSeed scaleMode
" Event identifiers.
syn keyword EventIdents type pos modifiers buttonType clicks delta key
syn keyword EventIdents text size focusType focused item
" Event aliases.
syn keyword EventIdents numClicks keys keyCode keyCodes textData
syn keyword EventIdents selectedItem entry
syn keyword EventIdents saveStateCount res dimmensions movieEndMode
syn keyword EventIdents onMovieEnd startingSeed rngSeed scaleMode

" Type member identifiers
syn match TimeIdents /[num]\=secs/
syn keyword PointIdents x y width height


syn cluster EventKeywords contains=EventType,@MouseKeywords,@KeyKeywords,TextType,@WindowKeywords
syn cluster MouseKeywords contains=MouseType,MouseButtonType,MouseButton
syn cluster KeyKeywords contains=KeyType,KeyModifiers,NonCharKeyCodes
syn cluster WindowKeywords contains=WindowType,WindowFocusType

syn cluster ShortEventKeywords contains=ShortEventType,@ShortMouseKeywords,@ShortKeyKeywords,ShortTextType,@ShortWindowKeywords
syn cluster ShortMouseKeywords contains=ShortMouseType,ShortMouseButtonType,ShortMouseButton
syn cluster ShortKeyKeywords contains=ShortKeyType,ShortKeyModifiers
syn cluster ShortWindowKeywords contains=ShortWindowType,ShotWindowFocusType

syn cluster MiscKeywords contains=ScaleMode,MovieEndMode

syn cluster Idents contains=HeaderIdents,EventIdents

syn cluster Keywords contains=BlockKeyword,@EventKeywords,@ShortEventKeywords,@MiscKeywords,@Idents

syn keyword Directives lsm version simple normal contained

syn keyword BoolValues true false


syn case ignore
syn match EscapeError /\\./ display contained
syn match escape /\\\([nrt\\'"]\|[xX]\x\{2}\)/ display contained
syn match escape /\\\(u\x\{4}\|U\x\{6}\)/ contained

syn cluster Escape contains=escape,EscapeError

syn keyword todo TODO FIXME XXX BUG HACK contained
syn region comment start="/\*" end="\*/" extend contains=todo
syn match comment "//.*$" contains=todo

syn region block start=/{/ end=/}/ transparent fold
syn region bracket start=/(/ end=/)/ transparent fold

syn region char start=/'/ end=/'/ contains=@Escape
syn region str oneline start=/"/ skip=/\\\\\|\\"/ end=/"/ contains=@Escape

syn region str start=/"""/ end=/"""/ contains=@Escape

syn match number /[+-]\=\d\+/ contained display
syn match number /[+-]\=0x\x\+/ contained display
syn match number /[+-]\=0b[01]\+/ contained display
syn match UnitValue /[+-]\=\d\+\(px\|tw\|[num]\=s\)\>/ contained display
syn match numbers /\<\d/ contains=number,UnitValue
syn match dimensions /\<\d\+\(px\|tw\)\=\s*[xX]\s*\d\+\(px\|tw\)\=\>/ contains=number,UnitValue display
syn case match

syn cluster Value contains=number,dimensions,char,string,table,BoolValues

syn match directive /#[_[:alpha:]][_[:alnum:]]*/ contains=ALLBUT,Directives,@Value,@Keywords display

hi def link comment Comment
hi def link todo Todo
hi def link char Constant
hi def link str String
hi def link numbers Constant
hi def link UnitValue Constant
hi def link dimensions Number
hi def link BoolValues Constant
hi def link escape SpecialChar
hi def link EscapeError Error
hi def link directive PreProc

hi def link BlockKeyword Type

hi def link EventType Type
hi def link MouseType Constant
hi def link KeyType Constant
hi def link TextType Constant
hi def link WindowType Constant
hi def link MouseButtonType Constant
hi def link WindowFocusType Constant
hi def link MouseButton Constant
hi def link KeyModifier Constant
hi def link NonCharKeyCodes Constant

hi def link ShortEventType Type
hi def link ShortMouseType Constant
hi def link ShortKeyType Constant
hi def link ShortTextType Constant
hi def link ShortWindowType Constant
hi def link ShortMouseButtonType Constant
hi def link ShortWindowFocusType Constant
hi def link ShortMouseButton Constant
hi def link ShortKeyModifier Constant

hi def link ScaleMode Constant
hi def link MovieEndMode Constant

hi def link HeaderIdents Identifier
hi def link EventIdents Identifier
hi def link TimeIdents Identifier
hi def link PointIdents Identifier


let b:current_syntax = "lsm"

let &cpo = s:cpo_save
unlet s:cpo_save

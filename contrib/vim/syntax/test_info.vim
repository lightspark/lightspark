" Vim syntax file for Lightspark's `test_info` files.
" Language: `test_info`
" Maintainer: mr b0nk 500
" Initial Revision: December 20 2024
" Latest Revision: December 20 2024

if exists("b:current_syntax")
	finish
endif

let s:cpo_save = &cpo
set cpo&vim

" Keywords.
" Members.
syn keyword TestInfoMembers name description author numFrames
syn keyword TestInfoMembers tickRate outputPath ignore knownFailType
syn keyword TestInfoMembers playerOptions logFetch screenDPI usesAssert
" Member aliases.
syn keyword TestInfoMembers desc info authors frames frameCount
syn keyword TestInfoMembers knownFailure knownCrash dpi

" Enums.
syn keyword FailureType Fail Crash 
syn keyword FlashMode AIR Flash AvmPlus
" Enum aliases.
syn keyword FlashMode air flash avmplus FlashPlayer fp

" Type members.
syn match TimeMembers /[num]\=secs/
syn keyword PointMembers x y width height
syn keyword PlayerOptionMembers maxExecution viewportDimensions renderOptions
syn keyword PlayerOptionMembers hasAudio hasVideo flashMode
syn keyword ApproximationMembers numberPatterns epsilon maxRelative
syn keyword ViewportDimensionMembers size scale
syn keyword RenderOptionMembers required sampleCount
" Type member aliases.
syn keyword PlayerOptionMembers timeout viewport


syn cluster EventKeywords contains=EventType,@MouseKeywords,@KeyKeywords,TextType,@WindowKeywords
syn cluster MouseKeywords contains=MouseType,MouseButtonType,MouseButton
syn cluster KeyKeywords contains=KeyType,KeyModifiers,NonCharKeyCodes
syn cluster WindowKeywords contains=WindowType,WindowFocusType

syn cluster ShortEventKeywords contains=ShortEventType,@ShortMouseKeywords,@ShortKeyKeywords,ShortTextType,@ShortWindowKeywords
syn cluster ShortMouseKeywords contains=ShortMouseType,ShortMouseButtonType,ShortMouseButton
syn cluster ShortKeyKeywords contains=ShortKeyType,ShortKeyModifiers
syn cluster ShortWindowKeywords contains=ShortWindowType,ShotWindowFocusType

syn cluster Enums contains=FailureType,FlashMode
syn cluster BaseTypeMembers contains=TimeMembers,PointMembers
syn cluster TypeMembers contains=PlayerOptionMembers,ViewportDimensionMembers,RenderOptionMembers
syn cluster Members contains=TestInfoMembers,@BaseTypeMembers,@TypeMembers
syn cluster Keywords contains=@Members,@Enums

syn keyword Directives test contained

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

hi def link FailureType Constant
hi def link FlashMode Constant

hi def link TestInfoMembers Identifier
hi def link TimeMembers Identifier
hi def link PointMembers Identifier
hi def link PlayerOptionMembers Identifier
hi def link ApproximationMembers Identifier
hi def link ViewportDimensionMembers Identifier
hi def link RenderOptionMembers Identifier


let b:current_syntax = "test_info"

let &cpo = s:cpo_save
unlet s:cpo_save

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

#include "compat.h"
#include "parsing/tags.h"
#include "frame.h"
#include "logger.h"
#include <vector>

namespace lightspark
{

class ExecutionContext
{
protected:
	int jumpOffset;
public:
	std::vector<ASObject*> regs;
	ASObject* retValue;
	ExecutionContext():jumpOffset(0),regs(100){}
	void setJumpOffset(int offset)
	{
		jumpOffset=offset;
	}
};

class ActionTag
{
public:
	int Length;
	ActionTag():Length(1){}
	virtual void Execute()=0;
	virtual void print()=0;
	virtual ~ActionTag(){}
};

class DoActionTag: public DisplayListTag, public ExecutionContext, public DisplayObject
{
private:
	std::vector<ActionTag*> actions;
public:
	DoActionTag(RECORDHEADER h, std::istream& in);
	void execute(MovieClip* parent, std::list < std::pair<PlaceInfo, DisplayObject*> >& list);
	void Render(bool maskEnabled);
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
	{
		//abort();
		return false;
	}
};

//This should be a control tag
class DoInitActionTag: public DisplayListTag, public ExecutionContext, public DisplayObject
{
private:
	UI16 SpriteID;

	std::vector<ActionTag*> actions;
	bool done;
public:
	DoInitActionTag(RECORDHEADER h, std::istream& in);
	void execute(MovieClip* parent, std::list < std::pair<PlaceInfo, DisplayObject*> >& list);
	void Render(bool maskEnabled);
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
	{
		//abort();
		return false;
	}
};

class ExportAssetsTag: public Tag
{
private:
	UI16 Count;
	std::vector<UI16> Tags;
	std::vector<STRING> Names;
public:
	ExportAssetsTag(RECORDHEADER h, std::istream& in);
};

class ACTIONRECORDHEADER
{
public:
	UI8 ActionCode;
	UI16 Length;
	ACTIONRECORDHEADER(std::istream& in);
	ActionTag* createTag(std::istream& in);
};

class ActionGetMember:public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionGetMember"));}
};

class ActionSetMember:public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionSetMember"));}
};

class ActionPlay:public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionPlay"));}
};

class ActionStop:public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionStop"));}
};

class ActionDefineFunction:public ActionTag, public IFunction
{
private:
	STRING FunctionName;
	UI16 NumParams;
	std::vector<STRING> params;
	UI16 CodeSize;

	std::vector<ActionTag*> functionActions;
public:
	ActionDefineFunction(std::istream& in,ACTIONRECORDHEADER* h);
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionDefineFunction"));}
	ASObject* call(ASObject* obj, ASObject* const* args, uint32_t num_args, bool thisOverride=false)
	{
		abort();
		return false;
	}
	STRING getName(){ return FunctionName; }
};

class REGISTERPARAM
{
public:
	UI8 Register;
	STRING ParamName;
};

class ActionDefineFunction2:public ActionTag, public IFunction, public ExecutionContext
{
private:
	STRING FunctionName;
	UI16 NumParams;
	UI8 RegisterCount;
	UB PreloadParentFlag;
	UB PreloadRootFlag;
	UB SuppressSuperFlag;
	UB PreloadSuperFlag;
	UB SuppressArgumentsFlag;
	UB PreloadArgumentsFlag;
	UB SuppressThisFlag;
	UB PreloadThisFlag;
	UB PreloadGlobalFlag;
	std::vector<REGISTERPARAM> Parameters;
	UI16 CodeSize;

	std::vector<ActionTag*> functionActions;
public:
	ActionDefineFunction2(std::istream& in,ACTIONRECORDHEADER* h);
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionDefineFunction2"));}
	ASObject* call(ASObject* obj, ASObject* const* args, uint32_t num_args, bool thisOverride=false)
	{
		abort();
		return false;
	}
	STRING getName(){ return FunctionName; }
	IFunction* toFunction(){ return this; }
};

class ActionJump:public ActionTag
{
private:
	SI16 BranchOffset;
public:
	ActionJump(std::istream& in);
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionJump"));}
};

class ActionWith:public ActionTag
{
private:
	UI16 Size;
public:
	ActionWith(std::istream& in);
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionWith"));}
};

class ActionIf:public ActionTag
{
private:
	SI16 Offset;
public:
	ActionIf(std::istream& in);
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionIf"));}
};

class ActionGotoFrame:public ActionTag
{
private:
	UI16 Frame;
public:
	ActionGotoFrame(std::istream& in);
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionGotoFrame"));}
};

class ActionGetURL2:public ActionTag
{
private:
	UI8 Reserved;
public:
	ActionGetURL2(std::istream& in);
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionGetURL2"));}
};

class ActionGetURL:public ActionTag
{
private:
	STRING URLString;
	STRING TargetString;
public:
	ActionGetURL(std::istream& in);
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionGetURL"));}
};

class ActionConstantPool : public ActionTag
{
private:
	UI16 Count;
	std::vector<STRING> ConstantPool;
public:
	ActionConstantPool(std::istream& in);
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionConstantPool"));}
};

class ActionSetTarget: public ActionTag
{
private:
	STRING TargetName;
public:
	ActionSetTarget(std::istream& in);  
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionSetTarget"));}
}; 

class ActionGoToLabel : public ActionTag
{
private:
	STRING Label;
public:
	ActionGoToLabel(std::istream& in);
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionGoToLabel"));}
};

class ActionStringAdd: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionStringAdd"));}
};

class ActionStringExtract: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionStringExtract"));}
};

class ActionNewObject: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionNewObject"));}
};

class ActionAdd2: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionAdd2"));}
};

class ActionModulo: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionModulo"));}
};

class ActionDivide: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionDivide"));}
};

class ActionPushDuplicate: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionPushDuplicate"));}
};

class ActionGetProperty: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionGetProperty"));}
};

class ActionReturn: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionReturn"));}
};

class ActionDefineLocal: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionDefineLocal"));}
};

class ActionMultiply: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionMultiply"));}
};

class ActionSubtract: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionSubtract"));}
};

class ActionPop: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionPop"));}
};

class ActionToInteger: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionToInteger"));}
};

class ActionNot: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionNot"));}
};

class ActionCallMethod: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionCallMethod"));}
};

class ActionCallFunction: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionCallFunction"));}
};

class ActionCloneSprite: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionCloneSprite"));}
};

class ActionTrace: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionTrace"));}
};

class ActionImplementsOp: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionImplementsOp"));}
};

class ActionExtends: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionExtends"));}
};

class ActionDecrement: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionDecrement"));}
};

class ActionInitObject: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionInitObject"));}
};

class ActionNewMethod: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionNewMethod"));}
};

class ActionDelete: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionDelete"));}
};

class ActionInitArray: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionInitArray"));}
};

class ActionTypeOf: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionTypeOf"));}
};

class ActionEnumerate: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionEnumerate"));}
};

class ActionGetTime: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionGetTime"));}
};

class ActionInstanceOf: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionInstanceOf"));}
};

class ActionSetProperty: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionSetProperty"));}
};

class ActionEnumerate2: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionEnumerate2"));}
};

class ActionToString: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionToString"));}
};

class ActionToNumber: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionToNumber"));}
};

class ActionCastOp: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionCastOp"));}
};

class ActionBitAnd: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionBitAnd"));}
};

class ActionBitOr: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionBitOr"));}
};

class ActionBitXor: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionBitXor"));}
};

class ActionBitLShift: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionBitLShift"));}
};

class ActionBitRShift: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionBitRShift"));}
};

class ActionIncrement: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionIncrement"));}
};

class ActionGreater: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionGreater"));}
};

class ActionStringGreater: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionStringGreater"));}
};

class ActionLess2: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionLess2"));}
};

class ActionAsciiToChar: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionAsciiToChar"));}
};

class ActionStrictEquals: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionStrictEquals"));}
};

class ActionEquals2: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionEquals2"));}
};

class ActionStringEquals: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionStringEquals"));}
};

class ActionSetVariable: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionSetVariable"));}
};

class ActionSetTarget2: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionSetTarget2"));}
};

class ActionNotImplemented: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("Not implemented action"));}
};

class ActionGetVariable: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionGetVariable"));}
};

class ActionToggleQuality: public ActionTag
{
public:
	void Execute();
};

class ActionStoreRegister : public ActionTag
{
private:
	UI8 RegisterNumber;
public:
	ActionStoreRegister(std::istream& in);
	void Execute();
	void print(){ LOG(LOG_TRACE,_("ActionStoreRegister"));}
};

class ActionPush : public ActionTag
{
private:
	UI8 Type;
/*	STRING String;
	//FLOAT Float;
	UI8 RegisterNumber;
	UI8 Boolean;
	DOUBLE Double;
	UI32 Integer;
	UI8 Constant8;
	UI16 Constant16;*/

	std::vector<ASObject*> Objects;
public:
	ActionPush(std::istream& in,ACTIONRECORDHEADER* h);
	void Execute();
	void print();
};

class BUTTONCONDACTION
{
public:
	UI16 CondActionSize;
	UB CondIdleToOverDown;
	UB CondOutDownToIdle;
	UB CondOutDownToOverDown;
	UB CondOverDownToOutDown;
	UB CondOverDownToOverUp;
	UB CondOverUpToOverDown;
	UB CondOverUpToIdle;
	UB CondIdleToOverUp;
	UB CondKeyPress;
	UB CondOverDownToIdle;
	std::vector<ActionTag*> Actions;
	
	bool isLast()
	{
		return !CondActionSize;
	}
};

std::istream& operator>>(std::istream& stream, BUTTONCONDACTION& v);

};

/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "tags.h"
#include "frame.h"
#include "logger.h"
#include <vector>

class ExecutionContext
{
protected:
	int jumpOffset;
public:
	std::vector<ISWFObject*> regs;
	ISWFObject* retValue;
	ExecutionContext():jumpOffset(0),regs(10){}
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
};

class DoActionTag: public DisplayListTag, public ExecutionContext
{
private:
	std::vector<ActionTag*> actions;
public:
	DoActionTag(RECORDHEADER h, std::istream& in);
	void Render( );
	int getDepth() const;
	void printInfo(int t=0);
};

class DoInitActionTag: public DisplayListTag, public ExecutionContext
{
private:
	UI16 SpriteID;

	std::vector<ActionTag*> actions;
	bool done;
public:
	DoInitActionTag(RECORDHEADER h, std::istream& in);
	void Render( );
	int getDepth() const;
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
	void print(){ LOG(TRACE,"ActionGetMember");}
};

class ActionSetMember:public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionSetMember");}
};

class ActionPlay:public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionPlay");}
};

class ActionStop:public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionStop");}
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
	SWFOBJECT_TYPE getObjectType()const{return T_FUNCTION;}
	void print(){ LOG(TRACE,"ActionDefineFunction");}
	ISWFObject* call(ISWFObject* obj, arguments* args);
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
	SWFOBJECT_TYPE getObjectType()const{return T_FUNCTION;}
	void print(){ LOG(TRACE,"ActionDefineFunction2");}
	ISWFObject* call(ISWFObject* obj, arguments* args);
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
	void print(){ LOG(TRACE,"ActionJump");}
};

class ActionWith:public ActionTag
{
private:
	UI16 Size;
public:
	ActionWith(std::istream& in);
	void Execute();
	void print(){ LOG(TRACE,"ActionWith");}
};

class ActionIf:public ActionTag
{
private:
	SI16 Offset;
public:
	ActionIf(std::istream& in);
	void Execute();
	void print(){ LOG(TRACE,"ActionIf");}
};

class ActionGotoFrame:public ActionTag
{
private:
	UI16 Frame;
public:
	ActionGotoFrame(std::istream& in);
	void Execute();
	void print(){ LOG(TRACE,"ActionGotoFrame");}
};

class ActionGetURL2:public ActionTag
{
private:
	UI8 Reserved;
public:
	ActionGetURL2(std::istream& in);
	void Execute();
	void print(){ LOG(TRACE,"ActionGetURL2");}
};

class ActionGetURL:public ActionTag
{
private:
	STRING UrlString;
	STRING TargetString;
public:
	ActionGetURL(std::istream& in);
	void Execute();
	void print(){ LOG(TRACE,"ActionGetURL");}
};

class ActionConstantPool : public ActionTag
{
private:
	UI16 Count;
	std::vector<STRING> ConstantPool;
public:
	ActionConstantPool(std::istream& in);
	void Execute();
	void print(){ LOG(TRACE,"ActionConstantPool");}
};

class ActionStringAdd: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionStringAdd");}
};

class ActionStringExtract: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionStringExtract");}
};

class ActionNewObject: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionNewObject");}
};

class ActionAdd2: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionAdd2");}
};

class ActionDivide: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionDivide");}
};

class ActionPushDuplicate: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionPushDuplicate");}
};

class ActionGetProperty: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionGetProperty");}
};

class ActionReturn: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionReturn");}
};

class ActionDefineLocal: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionDefineLocal");}
};

class ActionMultiply: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionMultiply");}
};

class ActionSubtract: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionSubtract");}
};

class ActionPop: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionPop");}
};

class ActionNot: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionNot");}
};

class ActionCallMethod: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionCallMethod");}
};

class ActionCallFunction: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionCallFunction");}
};

class ActionCloneSprite: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionCloneSprite");}
};

class ActionImplementsOp: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionImplementsOp");}
};

class ActionExtends: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionExtends");}
};

class ActionDecrement: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionDecrement");}
};

class ActionInitObject: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionInitObject");}
};

class ActionNewMethod: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionNewMethod");}
};

class ActionDelete: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionDelete");}
};

class ActionInitArray: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionInitArray");}
};

class ActionTypeOf: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionTypeOf");}
};

class ActionGetTime: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionGetTime");}
};

class ActionInstanceOf: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionInstanceOf");}
};

class ActionSetProperty: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionSetProperty");}
};

class ActionIncrement: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionIncrement");}
};

class ActionGreater: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionGreater");}
};

class ActionLess2: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionLess2");}
};

class ActionEquals2: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionEquals2");}
};

class ActionStringEquals: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionStringEquals");}
};

class ActionSetVariable: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionSetVariable");}
};

class ActionNotImplemented: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"Not implemented action");}
};

class ActionGetVariable: public ActionTag
{
public:
	void Execute();
	void print(){ LOG(TRACE,"ActionGetVariable");}
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
	void print(){ LOG(TRACE,"ActionStoreRegister");}
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

	std::vector<ISWFObject*> Objects;
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


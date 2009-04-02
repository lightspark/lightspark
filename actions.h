/**************************************************************************
    Lighspark, a free flash player implementation

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
#include <vector>

class ActionTag
{
public:
	virtual void Execute()=0;
	virtual void print()=0;
};

class DoActionTag: public DisplayListTag
{
private:
	std::vector<ActionTag*> actions;
public:
	DoActionTag(RECORDHEADER h, std::istream& in);
	void Render( );
	UI16 getDepth() const;
	void printInfo(int t=0);
};

class ACTIONRECORDHEADER
{
public:
	UI8 ActionCode;
	UI16 Length;
	ACTIONRECORDHEADER(std::istream& in);
	ActionTag* createTag(std::istream& in);
};

class ActionStop:public ActionTag
{
public:
	void Execute();
	void print(){ std::cerr  << "ActionStop" << std::endl;}
};

class ActionJump:public ActionTag
{
private:
	SI16 BranchOffset;
public:
	ActionJump(std::istream& in);
	void Execute();
	void print(){ std::cerr  << "ActionJump" << std::endl;}
};

class ActionIf:public ActionTag
{
private:
	SI16 Offset;
public:
	ActionIf(std::istream& in);
	void Execute();
	void print(){ std::cerr  << "ActionIf" << std::endl;}
};

class ActionGotoFrame:public ActionTag
{
private:
	UI16 Frame;
public:
	ActionGotoFrame(std::istream& in);
	void Execute();
	void print(){ std::cerr  << "ActionGotoFrame" << std::endl;}
};

class ActionGetURL:public ActionTag
{
private:
	STRING UrlString;
	STRING TargetString;
public:
	ActionGetURL(std::istream& in);
	void Execute();
	void print(){ std::cerr  << "ActionGetURL" << std::endl;}
};

class ActionConstantPool : public ActionTag
{
private:
	UI16 Count;
	std::vector<STRING> ConstantPool;
public:
	ActionConstantPool(std::istream& in);
	void Execute();
	void print(){ std::cerr  << "ActionConstantPool" << std::endl;}
};

class ActionStringAdd: public ActionTag
{
public:
	void Execute();
	void print(){ std::cerr  << "ActionStringAdd" << std::endl;}
};

class ActionStringExtract: public ActionTag
{
public:
	void Execute();
	void print(){ std::cerr  << "ActionStringExtract" << std::endl;}
};

class ActionNot: public ActionTag
{
public:
	void Execute();
	void print(){ std::cerr  << "ActionNot" << std::endl;}
};

class ActionStringEquals: public ActionTag
{
public:
	void Execute();
	void print(){ std::cerr  << "ActionStringEquals" << std::endl;}
};

class ActionSetVariable: public ActionTag
{
public:
	void Execute();
	void print(){ std::cerr  << "ActionSetVariable" << std::endl;}
};

class ActionGetVariable: public ActionTag
{
public:
	void Execute();
	void print(){ std::cerr  << "ActionGetVariable" << std::endl;}
};

class ActionToggleQuality: public ActionTag
{
public:
	void Execute();
};

class ActionPush : public ActionTag
{
private:
	UI8 Type;
	STRING String;
	//FLOAT Float;
	UI8 RegisterNumber;
	UI8 Boolean;
	//DOUBLE Double;
	UI32 Integer;
	UI8 Constant8;
	UI16 Constant16;
public:
	ActionPush(std::istream& in,ACTIONRECORDHEADER* h);
	void Execute();
	void print(){ std::cerr  << "ActionPush" << std::endl;}
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

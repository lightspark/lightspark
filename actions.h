#include "tags.h"
#include "frame.h"
#include <vector>

class RunState
{
public:
	int FP;
	int next_FP;
	bool stop_FP;
	sem_t sem_run;
	RunState();
};

class ActionTag
{
public:
	virtual void Execute()=0;
};

class DoActionTag: public DisplayListTag
{
private:
	std::vector<ActionTag*> actions;
public:
	DoActionTag(RECORDHEADER h, std::istream& in);
	void Render( );
	UI16 getDepth();
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
};

class ActionJump:public ActionTag
{
private:
	SI16 BranchOffset;
public:
	ActionJump(std::istream& in);
	void Execute();
};

class ActionIf:public ActionTag
{
private:
	SI16 Offset;
public:
	ActionIf(std::istream& in);
	void Execute();
};

class ActionGotoFrame:public ActionTag
{
private:
	UI16 Frame;
public:
	ActionGotoFrame(std::istream& in);
	void Execute();
};

class ActionGetURL:public ActionTag
{
private:
	STRING UrlString;
	STRING TargetString;
public:
	ActionGetURL(std::istream& in);
	void Execute();
};

class ActionConstantPool : public ActionTag
{
private:
	UI16 Count;
	std::vector<STRING> ConstantPool;
public:
	ActionConstantPool(std::istream& in);
	void Execute();
};

class ActionStringAdd: public ActionTag
{
public:
	void Execute();
};

class ActionStringExtract: public ActionTag
{
public:
	void Execute();
};

class ActionNot: public ActionTag
{
public:
	void Execute();
};

class ActionStringEquals: public ActionTag
{
public:
	void Execute();
};

class ActionSetVariable: public ActionTag
{
public:
	void Execute();
};

class ActionGetVariable: public ActionTag
{
public:
	void Execute();
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

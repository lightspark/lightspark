#include "tags.h"
#include "frame.h"
#include <vector>

class RunState
{
public:
	int FP;
	int next_FP;
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

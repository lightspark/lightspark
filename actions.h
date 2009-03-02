#include "tags.h"
#include "frame.h"
#include <vector>

class State
{
public:
	int FP;
	State();
	std::vector<Frame> frames;
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
};

class ActionStop:public ActionTag
{
public:
	void Execute();
};

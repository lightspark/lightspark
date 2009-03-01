#include "tags.h"

class DoActionTag: public DisplayListTag
{
public:
	DoActionTag(RECORDHEADER h, std::istream& in);
	void Render( );
	UI16 getDepth();
};

class ACTIONRECORDHEADER
{
private:
	UI8 ActionCode;
	UI16 Length;
public:
	ACTIONRECORDHEADER(std::istream& in);
};

class ActionTag
{
};

class 

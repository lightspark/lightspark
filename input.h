#ifndef INPUT_H
#define INPUT_H

#include "swf.h"

class IActiveObject
{
public:
	IActiveObject();
	virtual void MouseEvent(int x, int y)=0;
};


#endif

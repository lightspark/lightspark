#ifndef _FRAME_H
#define _FRAME_H

#include <list>
#include "swftypes.h"

class DisplayListTag;

class Frame
{
private:
	STRING Label;
public:
	std::list<DisplayListTag*> displayList;
	
	Frame(const std::list<DisplayListTag*>& d):displayList(d){ }
	void Render();
	void setLabel(STRING l);
};

#endif

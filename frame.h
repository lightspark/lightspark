#include <list>
#include "tags.h"

class Frame
{
public:
	std::list<DisplayListTag*> displayList;
	
	Frame(const std::list<DisplayListTag*>& d):displayList(d){}
};

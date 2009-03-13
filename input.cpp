#include "input.h"

IActiveObject::IActiveObject()
{
	InputThread::addListener(this);
}

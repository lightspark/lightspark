#include "flashevents.h"
#include <string>

using namespace std;

Event::Event(const string& t):type(t)
{
	setVariableByName("ENTER_FRAME",new ASString("enterFrame"));
	setVariableByName("ADDED_TO_STAGE",new ASString("addedToStage"));
}

MouseEvent::MouseEvent():Event("mouseEvent")
{
	setVariableByName("MOUSE_DOWN",new ASString("mouseDown"));
}

#include "action_back.h"

Action_Back::Action_Back() {
	connect(this, &QStreamDeckAction::keyDown, this, &Action_Back::onPressed);
}

void Action_Back::onPressed() {
	device()->switchToPreviousProfile();
}

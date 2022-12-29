#pragma once

#include "dvmaction.h"

class Action_Microphone : public DVMAction {

public:
	Action_Microphone();

public:
	virtual void update() override;

private slots:
	void onPressed();
	void onReleased();

private:
	int state_ = -1;

};

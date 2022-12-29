#pragma once

#include "dvmaction.h"

class Action_Deafen : public DVMAction {

public:
	Action_Deafen();

public:
	virtual void update() override;

private slots:
	void onPressed();
	void onReleased();

private:
	int state_ = -1;

};

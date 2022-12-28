#pragma once

#include "dvmaction.h"

class Action_OpenMixer : public DVMAction {
Q_OBJECT

public:
	Action_OpenMixer();

private slots:
	void onKeyDown();

};

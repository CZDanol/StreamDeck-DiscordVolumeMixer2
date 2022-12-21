#pragma once

#include <qtstreamdeck2/qstreamdeckaction.h>

class Action_OpenMixer : public QStreamDeckAction {

public:
	Action_OpenMixer();

private slots:
	void onKeyDown();

};

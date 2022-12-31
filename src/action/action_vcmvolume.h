#pragma once

#include <QTimer>

#include "voicechannelmemberaction.h"

class Action_VCMVolume : public VoiceChannelMemberAction {

public:
	Action_VCMVolume();

public:
	virtual void update() override;

protected:
	virtual void buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) override;

private slots:
	void onInitialized();
	void onPressed();
	void onReleased();
	void trigger();

private:
	bool isVolumeDown_ = false;
	int state_ = -1;

private:
	int repeatSkip_ = 0;
	QTimer repeatTimer_;

};

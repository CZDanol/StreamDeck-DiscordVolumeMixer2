#pragma once

#include "dvmaction.h"
#include "voicechannelmember.h"

class IndexedUserAction : public DVMAction {

public:
	IndexedUserAction();

public:
	int voiceChannelMemberIndex();
	VoiceChannelMember voiceChannelMember();

public slots:
	virtual void update() = 0;

protected:
	virtual void buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) override;

private slots:
	void onInitialized();

};

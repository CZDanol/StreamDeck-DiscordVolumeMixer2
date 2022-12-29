#pragma once

#include "dvmaction.h"
#include "voicechannelmember.h"

class IndexedVCMAction : public DVMAction {

public:
	IndexedVCMAction();

public:
	int voiceChannelMemberIndex();
	VoiceChannelMember &voiceChannelMember();

protected:
	virtual void buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) override;

private slots:
	void onInitialized();

};

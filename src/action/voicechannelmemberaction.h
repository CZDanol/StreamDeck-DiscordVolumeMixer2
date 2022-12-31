#pragma once

#include "dvmaction.h"
#include "voicechannelmember.h"

class VoiceChannelMemberAction : public DVMAction {

public:
	VoiceChannelMemberAction();

public:
	VoiceChannelMember *voiceChannelMember();

protected:
	virtual void buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) override;

private slots:
	void onInitialized();

};

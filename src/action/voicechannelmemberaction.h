#pragma once

#include "dvmaction.h"
#include "voicechannelmember.h"

class VoiceChannelMemberAction : public DVMAction {

public:
	VoiceChannelMemberAction();

public:
	struct VoiceChannelMemberResult {
		VoiceChannelMember *mem = nullptr;
		qsizetype userIndex = -1;

		inline operator bool() const {
			return mem;
		}
	};
	VoiceChannelMemberResult voiceChannelMember();

protected:
	virtual void buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) override;

private slots:
	void onInitialized();

};

#include "voicechannelmemberaction.h"

#include <qtstreamdeck2/qstreamdeckpropertyinspectorbuilder.h>

#include "dvmplugin.h"
#include "dvmdevice.h"

VoiceChannelMemberAction::VoiceChannelMemberAction() {
	connect(this, &QStreamDeckAction::initialized, this, &VoiceChannelMemberAction::onInitialized);
}

void VoiceChannelMemberAction::onInitialized() {
	// Convert from string to int (legacy reasons)
	if(const auto v = setting("user_ix"); v.isDouble())
		setSetting("user_ix", QString::number(v.toInt()));
}

VoiceChannelMember &VoiceChannelMemberAction::voiceChannelMember() {
	auto &lst = plugin()->voiceChannelMembers;

	QString userID = setting("user_ix").toString();

	// If the ID length is too low, that means that it's index in the list, not the actual user ID
	if(userID.length() < 4)
		userID = lst.keys().value(userID.toInt() + device()->voiceChannelMemberIndexOffset);

	auto it = lst.find(userID);

	static VoiceChannelMember invalidMember;
	return it != lst.end() ? *it : invalidMember;
}

void VoiceChannelMemberAction::buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) {
	b.addSpinBox("user_ix", "User index or ID").linkWithActionSetting();
	b.addMessage("Index in the channel users list, indexed from 0.");
	b.addMessage("Alternatively, you can also use User ID (right click user -> copy ID> to set it to a specific user.");

	DVMAction::buildPropertyInspector(b);
}

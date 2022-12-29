#include "indexedvcmaction.h"

#include <qtstreamdeck2/qstreamdeckpropertyinspectorbuilder.h>

#include "dvmplugin.h"
#include "dvmdevice.h"

IndexedVCMAction::IndexedVCMAction() {
	connect(this, &QStreamDeckAction::initialized, this, &IndexedVCMAction::onInitialized);
}

void IndexedVCMAction::onInitialized() {
	// Convert from string to int (legacy reasons)
	if(const auto v = setting("user_ix"); v.isString())
		setSetting("user_ix", v.toString().toInt());
}

int IndexedVCMAction::voiceChannelMemberIndex() {
	return setting("user_ix").toInt() + device()->voiceChannelMemberIndexOffset;
}

VoiceChannelMember &IndexedVCMAction::voiceChannelMember() {
	const int ix = voiceChannelMemberIndex();
	auto &lst = plugin()->voiceChannelMembers;
	auto it = lst.find(lst.keys().value(ix));

	static VoiceChannelMember invalidMember;
	return it != lst.end() ? *it : invalidMember;
}

void IndexedVCMAction::buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) {
	b.addSpinBox("user_ix", "User index").linkWithActionSetting();
	b.addMessage("Index in the channel users list, indexed from 0.");

	DVMAction::buildPropertyInspector(b);
}

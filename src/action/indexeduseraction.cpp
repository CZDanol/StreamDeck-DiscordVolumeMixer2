#include "indexeduseraction.h"

#include <qtstreamdeck2/qstreamdeckpropertyinspectorbuilder.h>

#include "dvmplugin.h"
#include "dvmdevice.h"

IndexedUserAction::IndexedUserAction() {
	connect(this, &QStreamDeckAction::initialized, this, &IndexedUserAction::onInitialized);
	connect(this, &QStreamDeckAction::settingsChanged, this, &IndexedUserAction::update);
}

void IndexedUserAction::onInitialized() {
	connect(plugin(), &DVMPlugin::buttonsUpdateRequested, this, &IndexedUserAction::update);

	// Convert from string to int (legacy reasons)
	if(const auto v = setting("user_ix"); v.isString())
		setSetting("user_ix", v.toString().toInt());

	update();
}

int IndexedUserAction::voiceChannelMemberIndex() {
	return setting("user_ix").toInt() + device()->voiceChannelMemberIndexOffset;
}

VoiceChannelMember IndexedUserAction::voiceChannelMember() {
	const int ix = voiceChannelMemberIndex();
	return plugin()->voiceChannelMembers.value(plugin()->voiceChannelMembers.keys().value(ix));
}

void IndexedUserAction::buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) {
	b.addSpinBox("user_ix", "User index").linkWithActionSetting();
	b.addMessage("Index in the channel users list, indexed from 0.");

	DVMAction::buildPropertyInspector(b);
}

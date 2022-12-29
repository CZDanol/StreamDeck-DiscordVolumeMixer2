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
	update();
}

int IndexedUserAction::voiceChannelMemberIndex() {
	return setting("user_ix").toInt() + device()->voiceChannelMemberIndexOffset;
}

VoiceChannelMember IndexedUserAction::voiceChannelMember() {
	return plugin()->voiceChannelMembers.value(plugin()->voiceChannelMembers.keys().value(voiceChannelMemberIndex()));
}

void IndexedUserAction::buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) {
	b.addSection("Indexed user action");
	b.addSpinBox("user_ix", "User index").linkWithActionSetting();
	b.addMessage("Index in the channel users list, indexed from 0.");

	DVMAction::buildPropertyInspector(b);
}

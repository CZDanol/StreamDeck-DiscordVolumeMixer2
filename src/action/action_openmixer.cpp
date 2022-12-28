#include "action_openmixer.h"

#include "dvmplugin.h"

Action_OpenMixer::Action_OpenMixer() {
	connect(this, &QStreamDeckAction::keyDown, this, &Action_OpenMixer::onKeyDown);
}

void Action_OpenMixer::onKeyDown() {
	using DT = QStreamDeckDevice::DeviceType;
	static const QMap<int, QString> profileNameByDeviceType{
		{+DT::streamDeck,       "Discord Volume Mixer"},
		{+DT::streamDeckMini,   "Discord Volume Mixer Mini"},
		{+DT::streamDeckXL,     "Discord Volume Mixer XL"},
		{+DT::streamDeckMobile, "Discord Volume Mixer"},
		{+DT::streamDeckPlus,   "Discord Volume Mixer+"},
	};

	device()->switchToProfile(profileNameByDeviceType.value(+device()->deviceType(), "Discord Volume Mixer"));
	plugin()->updateChannelMembersData();
}

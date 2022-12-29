#include "action_indexedvcmvolume.h"

#include <qtstreamdeck2/qstreamdeckpropertyinspectorbuilder.h>

#include "dvmplugin.h"

Action_IndexedVCMVolume::Action_IndexedVCMVolume() {
	connect(this, &QStreamDeckAction::initialized, this, &Action_IndexedVCMVolume::onInitialized);
	connect(this, &QStreamDeckAction::keyDown, this, &Action_IndexedVCMVolume::onPressed);
	connect(this, &QStreamDeckAction::keyUp, this, &Action_IndexedVCMVolume::onReleased);

	repeatTimer_.setInterval(100);
	repeatTimer_.callOnTimeout([this] {
		if(repeatSkip_-- <= 0)
			trigger();
	});
}

void Action_IndexedVCMVolume::update() {
	VoiceChannelMember &vcm = voiceChannelMember();

	const int targetState = int(!vcm.isValid || (isVolumeDown_ ? vcm.volume <= QDiscord::minVoiceVolume : vcm.volume >= QDiscord::maxVoiceVolume));
	if(state_ != targetState) {
		state_ = targetState;
		setState(state_);
	}
}

void Action_IndexedVCMVolume::buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) {
	b.addSpinBox("voiceChannelVolumeButtonStep", "Volume step").linkWithGlobalSetting();
	b.addMessage("Volume step is global for all volume buttons.");

	IndexedVCMAction::buildPropertyInspector(b);
}

void Action_IndexedVCMVolume::onInitialized() {
	isVolumeDown_ = (actionUID() == "cz.danol.discordmixer.volumedown");
}

void Action_IndexedVCMVolume::onPressed() {
	/// Ignore first 300 ms
	repeatSkip_ = 3;
	trigger();
	repeatTimer_.start();
}

void Action_IndexedVCMVolume::onReleased() {
	repeatTimer_.stop();

	// Force update state
	setState(state_);
}

void Action_IndexedVCMVolume::trigger() {
	VoiceChannelMember &vcm = voiceChannelMember();
	if(!vcm.isValid)
		return;

	const int stepSize = plugin()->globalSetting("voiceChannelVolumeButtonStep").toInt();
	const int numSteps = isVolumeDown_ ? -1 : 1;
	plugin()->adjustVoiceChannelMemberVolume(vcm, stepSize, numSteps);
}

#include "action_vcmvolume.h"

#include <qtstreamdeck2/qstreamdeckpropertyinspectorbuilder.h>

#include "dvmplugin.h"

Action_VCMVolume::Action_VCMVolume() {
	connect(this, &QStreamDeckAction::initialized, this, &Action_VCMVolume::onInitialized);
	connect(this, &QStreamDeckAction::keyDown, this, &Action_VCMVolume::onPressed);
	connect(this, &QStreamDeckAction::keyUp, this, &Action_VCMVolume::onReleased);

	repeatTimer_.setInterval(100);
	repeatTimer_.callOnTimeout([this] {
		if(repeatSkip_-- <= 0)
			trigger();
	});
}

void Action_VCMVolume::update() {
	const auto vcmp = voiceChannelMember();
	const VoiceChannelMember& vcm = vcmp ? *vcmp.mem : VoiceChannelMember::null;

	const int targetState = int(!vcmp || (isVolumeDown_ ? vcm.volume <= QDiscord::minVoiceVolume : vcm.volume >= QDiscord::maxVoiceVolume));
	if(state_ != targetState) {
		state_ = targetState;
		setState(state_);
	}
}

void Action_VCMVolume::buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) {
	b.addSpinBox("voiceChannelVolumeButtonStep", "Volume step").linkWithGlobalSetting();
	b.addMessage("Volume step is global for all volume buttons.");

	VoiceChannelMemberAction::buildPropertyInspector(b);
}

void Action_VCMVolume::onInitialized() {
	isVolumeDown_ = (actionUID() == "cz.danol.discordmixer.volumedown");
}

void Action_VCMVolume::onPressed() {
	/// Ignore first 300 ms
	repeatSkip_ = 3;
	trigger();
	repeatTimer_.start();
}

void Action_VCMVolume::onReleased() {
	repeatTimer_.stop();

	// Force update state
	setState(state_);
}

void Action_VCMVolume::trigger() {
	const auto vcm = voiceChannelMember();
	if(!vcm)
		return;

	const int stepSize = plugin()->globalSetting("voiceChannelVolumeButtonStep").toInt();
	const int numSteps = isVolumeDown_ ? -1 : 1;
	plugin()->adjustVoiceChannelMemberVolume(*vcm.mem, stepSize, numSteps);
}

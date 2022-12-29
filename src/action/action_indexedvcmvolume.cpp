#include "action_indexedvcmvolume.h"

#include <qtstreamdeck2/qstreamdeckpropertyinspectorbuilder.h>

#include "dvmplugin.h"

static constexpr float minVolume = 0;
static constexpr float maxVolume = 200;

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

	const int targetState = int(!vcm.isValid || (isVolumeDown_ ? vcm.volume <= minVolume : vcm.volume >= maxVolume));
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

	const float step = plugin()->globalSetting("voiceChannelVolumeButtonStep").toInt() * (isVolumeDown_ ? -1 : 1);
	const float newVolume = qRound(qBound(minVolume, vcm.volume + step, maxVolume) / step) * step;

	if(newVolume != vcm.volume || vcm.isMuted) {
		vcm.volume = newVolume;
		vcm.isMuted = false;
		state_ = -1;

		plugin()->discord.sendCommand(+QDiscord::CommandType::setUserVoiceSettings, QJsonObject{
			{"user_id", vcm.userID},
			{"volume",  QDiscord::uiToIPCVolume(newVolume)},
			{"mute",    false},
		});
		emit plugin()->buttonsUpdateRequested();
	}
}

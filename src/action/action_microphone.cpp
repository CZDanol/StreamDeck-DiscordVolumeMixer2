#include "action_microphone.h"

#include "dvmplugin.h"

Action_Microphone::Action_Microphone() {
	connect(this, &QStreamDeckAction::keyDown, this, &Action_Microphone::onPressed);
	connect(this, &QStreamDeckAction::keyUp, this, &Action_Microphone::onReleased);
}

void Action_Microphone::update() {
	if(const int newState = plugin()->isMicrophoneMuted; state_ != newState) {
		state_ = newState;
		setState(newState);
	}
}

void Action_Microphone::onPressed() {
	plugin()->isMicrophoneMuted ^= true;
	plugin()->discord.sendCommand(+QDiscord::CommandType::setVoiceSettings, {{"mute", plugin()->isMicrophoneMuted}});
	emit plugin()->buttonsUpdateRequested();
}

void Action_Microphone::onReleased() {
	setState(state_);
}

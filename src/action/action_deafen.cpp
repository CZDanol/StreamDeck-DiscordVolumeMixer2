#include "action_deafen.h"

#include "dvmplugin.h"

Action_Deafen::Action_Deafen() {
	connect(this, &QStreamDeckAction::keyDown, this, &Action_Deafen::onPressed);
	connect(this, &QStreamDeckAction::keyUp, this, &Action_Deafen::onReleased);
}

void Action_Deafen::update() {
	if(const int newState = plugin()->isDeafened; state_ != newState) {
		state_ = newState;
		setState(newState);
	}
}

void Action_Deafen::onPressed() {
	plugin()->isDeafened ^= true;
	plugin()->isMicrophoneMuted |= plugin()->isDeafened;
	plugin()->discord.sendCommand(+QDiscord::CommandType::setVoiceSettings, {{"deaf", plugin()->isDeafened}});
	emit plugin()->buttonsUpdateRequested();
}

void Action_Deafen::onReleased() {
	setState(state_);
}

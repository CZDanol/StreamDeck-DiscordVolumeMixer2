#include "dvmplugin.h"

#include "dvmdevice.h"

#include "action/action_openmixer.h"

DVMPlugin::DVMPlugin() : settings("Danol", "Stream Deck Discord Volume Mixer") {
	registerActionType<Action_OpenMixer>("cz.danol.discordmixer.openmixer");
}

DVMPlugin::~DVMPlugin() {

}

void DVMPlugin::updateChannelMembersData() {

}

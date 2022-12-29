#include "dvmplugin.h"

#include "dvmdevice.h"

#include "action/action_openmixer.h"
#include "action/action_indexeduser.h"

DVMPlugin::DVMPlugin() {
	registerActionType<Action_OpenMixer>("cz.danol.discordmixer.openmixer");
	registerActionType<Action_IndexedUser>("cz.danol.discordmixer.user");

	connect(this, &QStreamDeckPlugin::eventReceived, this, &DVMPlugin::onStreamDeckEventReceived);

	connect(&discord, &QDiscord::messageReceived, this, &DVMPlugin::onDiscordMessageReceived);
	connect(&discord, &QDiscord::avatarReady, this, &DVMPlugin::buttonsUpdateRequested);

	connectToDiscord();
}

DVMPlugin::~DVMPlugin() {

}

void DVMPlugin::connectToDiscord() {
	if(discord.isConnected())
		return;

	if(discord.connect(globalSetting("client_id").toString(), globalSetting("client_secret").toString())) {

		// Subscribe to voice channel select event
		discord.sendCommand(+QDiscord::CommandType::subscribe, {}, QJsonObject{
			{"evt", "VOICE_CHANNEL_SELECT"}
		});

		updateChannelMembersData();
	}
	else
		emit buttonsUpdateRequested();
}

void DVMPlugin::updateChannelMembersData() {
	QDiscordReply *r = discord.sendCommand(+QDiscord::CommandType::getSelectedVoiceChannel);
	connect(r, &QDiscordReply::success, this, [this](const QDiscordMessage &msg) {
		const QString voiceChannelID = msg.json["data"]["id"].toString();

		// If the channel changed, update event subscribtions
		if(voiceChannelID != currentVoiceChannelID) {
			static const QStringList events{
				"VOICE_STATE_UPDATE", "VOICE_STATE_CREATE", "VOICE_STATE_DELETE", "SPEAKING_START", "SPEAKING_STOP"
			};
			const auto evf = [&](const QString &cmd) {
				const QJsonObject args{{"channel_id", voiceChannelID}};
				for(const QString &e: events)
					discord.sendCommand(cmd, args, {{"evt", e}});
			};

			if(!currentVoiceChannelID.isNull())
				evf(+QDiscord::CommandType::unsubscribe);

			currentVoiceChannelID = voiceChannelID;

			if(!currentVoiceChannelID.isNull())
				evf(+QDiscord::CommandType::subscribe);
		}

		// Update voice channel member list
		{
			const auto arr = msg.json["data"]["voice_states"].toArray();

			voiceChannelMembers.clear();

			for(const auto &v: arr) {
				const VoiceChannelMember vs = VoiceChannelMember::fromJson(v.toObject());
				if(vs.userID != discord.userID())
					voiceChannelMembers.insert(vs.userID, vs);
			}
		}

		if(voiceChannelMemberIxOffset >= voiceChannelMembers.size())
			voiceChannelMemberIxOffset = 0;

		emit buttonsUpdateRequested();
	});
}

void DVMPlugin::updateSelfVoiceState(const QDiscordMessage &msg) {
	const QJsonObject json = msg.json["data"].toObject();

	if(auto v = json["voice_state"]["self_mute"]; !v.isNull())
		isMicrophoneMuted = v.toBool();

	if(auto v = json["voice_state"]["self_deaf"]; !v.isNull())
		isDeafened = v.toBool();
}

void DVMPlugin::onDiscordMessageReceived(const QDiscordMessage &msg) {
	using ET = QDiscordMessage::EventType;

	switch(msg.event) {

		// Voice channel changed
		case ET::voiceChannelSelect:
			updateChannelMembersData();
			break;

		case ET::voiceStateCreate: {
			const auto m = VoiceChannelMember::fromJson(msg.json["data"].toObject());
			if(m.userID == discord.userID())
				updateSelfVoiceState(msg);

			else
				voiceChannelMembers.insert(m.userID, m);

			break;
		}

		case ET::voiceStateUpdate: {
			const auto m = VoiceChannelMember::fromJson(msg.json["data"].toObject());
			if(m.userID == discord.userID())
				updateSelfVoiceState(msg);

			else if(voiceChannelMembers.contains(m.userID))
				voiceChannelMembers.insert(m.userID, m);

			break;
		}

		case ET::voiceStateDelete: {
			voiceChannelMembers.remove(VoiceChannelMember::fromJson(msg.json["data"].toObject()).userID);
			if(voiceChannelMemberIxOffset >= voiceChannelMembers.size())
				voiceChannelMemberIxOffset = 0;

			break;
		}

		case ET::speakingStart:
			speakingVoiceChannelMembers.insert(msg.json["data"]["user_id"].toString());
			break;

		case ET::speakingStop:
			speakingVoiceChannelMembers.remove(msg.json["data"]["user_id"].toString());
			break;

		default:
			return;

	}

	emit buttonsUpdateRequested();
}

void DVMPlugin::onStreamDeckEventReceived(const QStreamDeckEvent &e) {
	using ET = QStreamDeckEvent::EventType;

	// Try connecting to discord whenever any button is pressed
	if(!discord.isConnected())
		connectToDiscord();
}

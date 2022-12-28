#include "dvmplugin.h"

#include "dvmdevice.h"

#include "action/action_openmixer.h"

DVMPlugin::DVMPlugin() {
	registerActionType<Action_OpenMixer>("cz.danol.discordmixer.openmixer");

	connect(&discord, &QDiscord::messageReceived, this, &DVMPlugin::onDiscordMessageReceived);
}

DVMPlugin::~DVMPlugin() {

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
			speakingUsers.insert(msg.json["data"]["user_id"].toString());
			break;

		case ET::speakingStop:
			speakingUsers.remove(msg.json["data"]["user_id"].toString());
			break;

		default:
			return;

	}

	emit buttonsUpdateRequested();
}

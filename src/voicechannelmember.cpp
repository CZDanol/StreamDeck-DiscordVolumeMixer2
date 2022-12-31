#include "voicechannelmember.h"

#include <qtdiscordipc/qdiscord.h>

const VoiceChannelMember VoiceChannelMember::null;

VoiceChannelMember VoiceChannelMember::fromJson(const QJsonObject &json) {
	return VoiceChannelMember{
		.nick = json["nick"].toString(),
		.userID = json["user"]["id"].toString(),
		.avatarID = json["user"]["avatar"].toString(),
		.volume = float(qRound(QDiscord::ipcToUIVolume(json["volume"].toDouble()))),
		.isMuted = json["mute"].toBool(),
	};
}

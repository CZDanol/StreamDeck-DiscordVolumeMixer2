#include "voicechannelmember.h"

#include <qtdiscordipc/qdiscord.h>

const VoiceChannelMember VoiceChannelMember::null;

VoiceChannelMember VoiceChannelMember::fromJson(const QJsonObject &json) {
	const QJsonObject user = json["user"].toObject();
	QString displayName = json["nick"].toString().trimmed();
	if(displayName.isEmpty())
		displayName = user["username"].toString().trimmed();

	return VoiceChannelMember{
		.nick = displayName,
		.userID = user["id"].toString(),
		.avatarID = user["avatar"].toString(),
		.volume = float(qRound(QDiscord::ipcToUIVolume(json["volume"].toDouble()))),
		.isMuted = json["mute"].toBool(),
	};
}

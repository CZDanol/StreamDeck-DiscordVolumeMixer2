#pragma once

#include <QString>
#include <QJsonObject>

class VoiceChannelMember {

public:
	static VoiceChannelMember fromJson(const QJsonObject &json);

public:
	QString nick;
	QString userID, avatarID;
	float volume = 0;
	bool isMuted = false;
	bool isValid = false;

};

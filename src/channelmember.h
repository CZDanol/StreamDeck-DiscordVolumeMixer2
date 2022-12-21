#pragma once

#include <QString>

class ChannelMember {

public:
	QString nick;
	QString userID, avatarID;
	float volume = 0;
	bool isMuted = false;

};

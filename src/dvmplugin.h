#pragma once

#include <qtstreamdeck2/qstreamdeckplugin.h>
#include <qtdiscordipc/qdiscord.h>
#include <QSettings>

#include "declares.h"
#include "voicechannelmember.h"
#include "dvmdevice.h"

class DVMPlugin : public QStreamDeckPluginT<DVMDevice> {
Q_OBJECT

public:
	DVMPlugin();
	~DVMPlugin();

public slots:
	/// Attempts to connect to Discord
	void connectToDiscord();

	/// Reloads all channel member data
	void updateChannelMembersData();

public:
	/// Processes voice state update for the user itself
	void updateSelfVoiceState(const QDiscordMessage &msg);

public:
	QDiscord discord;

public:
	QString currentVoiceChannelID;
	QMap<QString, VoiceChannelMember> voiceChannelMembers;
	QSet<QString> speakingVoiceChannelMembers;

	int voiceChannelMemberIxOffset = 0;

public:
	bool isDeafened = false;
	bool isMicrophoneMuted = false;

signals:
	/// Updates text & states of all user related buttons
	void buttonsUpdateRequested();

private slots:
	void onDiscordMessageReceived(const QDiscordMessage &msg);
	void onStreamDeckEventReceived(const QStreamDeckEvent &e);

};

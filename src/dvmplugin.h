#pragma once

#include <qtstreamdeck2/qstreamdeckplugin.h>
#include <qtdiscordipc/qdiscord.h>

#include "declares.h"
#include "channelmember.h"

class DVMPlugin : public QStreamDeckPluginT<DVMPluginDevice> {
Q_OBJECT

public:
	DVMPlugin();

public:
	/// Reloads all channel member data
	void updateChannelMembersData();

public:
	QDiscord discord;

public:
	QMap<QString, ChannelMember> channelMembers;

private slots:
	void onDiscordMessageReceived(const QJsonObject &json);

};

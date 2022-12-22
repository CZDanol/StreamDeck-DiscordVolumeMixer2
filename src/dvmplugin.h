#pragma once

#include <qtstreamdeck2/qstreamdeckplugin.h>
#include <qtdiscordipc/qdiscord.h>
#include <QSettings>

#include "declares.h"
#include "channelmember.h"

class DVMPlugin : public QStreamDeckPluginT<DVMDevice> {
Q_OBJECT

public:
	DVMPlugin();
	~DVMPlugin();

public:
	/// Reloads all channel member data
	void updateChannelMembersData();

public:
	QDiscord discord;

	/// I'm using Qt settings library instead of the StreamDeck globalSettings functionality.
	/// With the SD globalSetttings you have to asynchronously ask and wait to it so it's not available right away, which is a bit annoying.
	QSettings settings;

public:
	QMap<QString, ChannelMember> channelMembers;

private slots:
	void onDiscordMessageReceived(const QJsonObject &json);

};

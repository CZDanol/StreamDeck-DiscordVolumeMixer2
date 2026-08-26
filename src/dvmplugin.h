#pragma once

#include <qtstreamdeck2/qstreamdeckplugin.h>
#include <qtdiscordipc/qdiscord.h>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QPixmap>
#include <QHash>
#include <QSet>

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

	void adjustVoiceChannelMemberVolume(VoiceChannelMember &vcm, float stepSize, int numSteps);

	/// Returns a cached avatar, or starts a non-blocking CDN download and returns null.
	QPixmap getUserAvatar(const QString &userId, const QString &avatarHash);

	/// Short, user-facing connection state suitable for Stream Deck keys.
	QString discordStatusText() const;

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
	bool isDiscordConnecting = false;

signals:
	/// Updates text & states of all user related buttons
	void buttonsUpdateRequested();

private:
	void updateCurrentVoiceChannel(const QString &newVoiceChannel);
	void scheduleDiscordReconnect();
	void migrateLegacySettings();
	void purgeOauthCache();
	QByteArray credentialsFingerprint() const;

private slots:
	void onInitialized();
	void onDiscordMessageReceived(const QDiscordMessage &msg);
	void onStreamDeckEventReceived(const QStreamDeckEvent &e);

private:
	QTimer discordConnectTimeoutTimer_;
	QTimer discordReconnectTimer_;
	QTimer credentialsReconnectTimer_;
	int discordReconnectFailures_ = 0;
	bool streamDeckReady_ = false;
	QByteArray observedCredentialsFingerprint_;
	QString rejectedClientID_;
	qint64 rejectedClientRetryAfter_ = 0;
	QString oauthRecoveryClientID_;
	qint64 lastManualReconnectAttempt_ = 0;
	QString lastConnectionError_;

	QNetworkAccessManager avatarNetworkManager_;
	QHash<QString, QPixmap> avatarCache_;
	QSet<QString> avatarDownloadsInFlight_;
	QHash<QString, qint64> avatarRetryAfter_;

};

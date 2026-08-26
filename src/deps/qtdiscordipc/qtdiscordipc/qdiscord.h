#pragma once

#include <QObject>
#include <QLocalSocket>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QImage>
#include <QCache>
#include <QNetworkAccessManager>
#include <QSet>

#include <optional>

#include "qdiscordmessage.h"
#include "qdiscordreply.h"

class QDiscord : public QObject {
Q_OBJECT

public:
	static constexpr float minVoiceVolume = 0;
	static constexpr float maxVoiceVolume = 200;

public:
	enum class CommandType {
		unknown = -1,
		dispatch,
		authorize,
		authenticate,

		getGuild,
		getGuilds,
		getChannel,
		getChannels,

		subscribe,
		unsubscribe,

		setUserVoiceSettings,
		selectVoiceChannel,
		getSelectedVoiceChannel,
		selectTextChannel,
		setVoiceSettings,
		getVoiceSettings,
		setCertifiedDevices,

		setActivity,
		sendActivityJoinInvite,
		closeActivityRequest,
	};

	Q_ENUM(CommandType);

public:
	QDiscord();
	~QDiscord();

public:
	/// Maps volume value returned by the IPC to the value that is shown in the Discord UI
	static double ipcToUIVolume(double v);

	/// Maps volume value that is shown in the Discord UI to the value range the IPC uses
	static double uiToIPCVolume(double v);

public:
	/**
	 * Tries to connext to the Discord. Returns true if successfull (this function is blocking)
	 */
	bool connect(const QString &clientID, const QString &clientSecret,
	             const QString &redirectUri = QStringLiteral("http://localhost:1337/callback"));

	void disconnect();

	inline bool isConnected() const {
		return isConnected_;
	}

	inline const QString &connectionError() const {
		return connectionError_;
	}

	inline const QString &userID() const {
		return userID_;
	}

	/// Returns whether the discord is processing something (connecting, waiting for message, ...)
	inline bool isProcessing() const {
		return processing_ > 0;
	}

public:
	/**
	 * Sends a command. Asynchronously returns the result via QDiscordReply::finished
	 * $args is put in the "args" field.
	 * $msgOverrides is injected into the main body
	 */
	QDiscordReply *sendCommand(const QString &command, const QJsonObject &args = {}, const QJsonObject &msgOverrides = {});

public:
	/// The function can be async, the avatar loading can be delayed and then signalled using avatarReady
	QImage getUserAvatar(const QString &userId, const QString &avatarId);

signals:
	/// This signal is emitted when there is a message received that is not a response to a command
	void messageReceived(const QDiscordMessage &msg);

	void avatarReady(const QString &avatarId, const QImage &img);

	void connected();

	void disconnected();

private:
	struct IpcFrame {
		quint32 opcode = 0;
		QByteArray payload;
	};

	/// Blockingly reads a single Discord frame message.
	std::optional<QDiscordMessage> readMessage(int timeoutMs = 20000);
	std::optional<IpcFrame> readFrame(int timeoutMs);
	std::optional<IpcFrame> takeBufferedFrame();

	void sendMessage(const QJsonObject &packet, int opCode = 1);
	void sendFrame(const QByteArray &payload, quint32 opCode);

	/**
 * Processes incoming messages.
 */
	void processMessage(const QDiscordMessage &msg);

private:
	/// Non-blocking processes received messsages
	void readAndProcessMessages();
	void processFrame(const IpcFrame &frame);
	void failPendingReplies(const QString &message);
	void setCloseError(const QJsonObject &payload);

private:
	QLocalSocket socket_;
	bool isConnected_ = false;
	QString connectionError_;
	QString userID_;
	QString cdn_;
	int nonceCounter_ = 0;
	int blockingRead_ = 0;
	int processing_ = 0;
	QByteArray receiveBuffer_;

private:
	QNetworkAccessManager netMgr_;
	QCache<QString, QImage> avatarsCache_;
	QSet<QString> avatarsInFlight_;
	QHash<QString, qint64> avatarRetryAfter_;
	QHash<QString, QDiscordReply *> pendingReplies_;

};

QString operator +(QDiscord::CommandType ct);

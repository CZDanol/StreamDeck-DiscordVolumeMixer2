#pragma once

#include <QObject>
#include <QJsonObject>

struct QDiscordMessage {
Q_GADGET

public:
	enum class EventType {
		unknown,
		ready,
		error,
		guildStatus,
		guildCreate,
		channelCreate,
		voiceChannelSelect,
		voiceStateCreate,
		voiceStateUpdate,
		voiceStateDelete,
		voiceSettingsUpdate,
		voiceConnectionStatus,
		speakingStart,
		speakingStop,
		messageCreate,
		messageUpdate,
		messageDelete,
		notificationCreate,
		activityJoin,
		activitySpectate,
		activityJoinRequest,
	};

	Q_ENUM(EventType);

public:
	static QDiscordMessage fromJson(const QJsonObject &json, int opcode = 0);

public:
	EventType event = EventType::unknown;
	QJsonObject json, data;
	QString nonce;
	int opcode = 0;

};

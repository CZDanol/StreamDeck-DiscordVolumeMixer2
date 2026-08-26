#include "qdiscordmessage.h"

#include <QMetaEnum>
#include <QRegularExpression>

QDiscordMessage QDiscordMessage::fromJson(const QJsonObject &json, int opcode) {
	static const QHash<QString, EventType> eventTypes = [] {
		const auto me = QMetaEnum::fromType<EventType>();
		const int cnt = me.keyCount();

		const QRegularExpression regex("([A-Z])");

		QHash<QString, EventType> r;
		r.reserve(cnt);
		for(int i = 0; i < cnt; i++) {
			QString key = me.key(i);
			key.replace(regex, "_\\1"); // Add _ underscore before every capitalized letter (guildStatus -> guild_Status)
			key = key.toUpper(); // Convert all to uppercase (guild_Status -> GUILD_STATUS)

			r.insert(key, EventType(me.value(i)));
		}

		return r;
	}();

	return QDiscordMessage{
		.event = eventTypes.value(json["evt"].toString()),
		.json = json,
		.data = json["data"].toObject(),
		.nonce = json["nonce"].toString(),
		.opcode = opcode,
	};
}

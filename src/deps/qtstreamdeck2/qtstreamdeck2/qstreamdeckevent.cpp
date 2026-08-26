#include "qstreamdeckevent.h"

#include <QMetaEnum>
#include <QJsonArray>

QStreamDeckEvent QStreamDeckEvent::fromMessage(const QJsonObject &json) {
	static const QHash<QString, EventType> eventTypes = []() {
		QHash<QString, EventType> r;
		const auto me = QMetaEnum::fromType<EventType>();
		const auto cnt = me.keyCount();
		r.reserve(cnt);

		for(int i = 0; i < cnt; i++)
			r[me.key(i)] = EventType(me.value(i));

		return r;
	}();

	return QStreamDeckEvent{
		.json = json,
		.eventType = eventTypes.value(json["event"].toString()),
		.payload = json["payload"].toObject(),
	};
}
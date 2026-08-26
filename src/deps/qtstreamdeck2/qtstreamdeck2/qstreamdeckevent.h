#pragma once

#include <QObject>
#include <QJsonObject>
#include <QPoint>

#include "qstreamdeckdeclares.h"

struct QStreamDeckEvent {
Q_GADGET

public:
	enum class EventType {
		unknown = -1,
		didReceiveSettings,
		didReceiveGlobalSettings,
		keyDown,
		keyUp,
		touchTap,
		dialDown,
		dialUp,
		dialRotate,
		willAppear,
		willDisappear,
		titleParametersDidChange,
		deviceDidConnect,
		deviceDidDisconnect,
		applicationDidLaunch,
		applicationDidTerminate,
		systemDidWakeUp,
		propertyInspectorDidAppear,
		propertyInspectorDidDisappear,
		sendToPlugin
	};

	Q_ENUM(EventType);

public:
	static QStreamDeckEvent fromMessage(const QJsonObject &json);

public:
	/// Original json object of the event
	QJsonObject json;

	EventType eventType = EventType::unknown;
	QJsonObject payload;

};

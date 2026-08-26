#include "qstreamdeckaction.h"

#include <QMetaEnum>
#include <QJsonArray>
#include <QImage>

#include "qstreamdeckplugin.h"
#include "qstreamdeckcommand.h"
#include "qstreamdeckpropertyinspectorbuilder.h"

QStreamDeckAction::QStreamDeckAction() {
	connect(this, &QStreamDeckAction::eventReceived, this, &QStreamDeckAction::onEventReceived);
}

QStreamDeckAction::~QStreamDeckAction() {
	device_->plugin()->actions_.remove(actionContext_);
}

void QStreamDeckAction::init(QStreamDeckDevice *device, const QStreamDeckEvent &appearEvent) {
	const QJsonObject &json = appearEvent.json;
	const QJsonObject &payload = appearEvent.payload;

	device_ = device;
	actionContext_ = json["context"].toString();
	actionUID_ = json["action"].toString();
	settings_ = payload["settings"].toObject();

	state_ = payload["state"].toInt();
	isInMultiAction_ = payload["isInMultiAction"].toBool();

	const auto coordJson = payload["coordinates"];
	coordinates_ = QPoint(coordJson["column"].toInt(), coordJson["row"].toInt());

	const auto controllerStr = payload["controller"].toString().toLower();
	controller_ = Controller(QMetaEnum::fromType<Controller>().keyToValue(controllerStr.toStdString().c_str()));

	plugin()->actions_.insert(actionContext_, this);

	emit initialized();
}

void QStreamDeckAction::setSettings(const QJsonObject &set) {
	settings_ = set;

	plugin()->sendMessage(QJsonObject{
		{"event",   +QStreamDeckCommand::setSettings},
		{"context", actionContext_},
		{"payload", settings_},
	});

	emit settingsChanged();
}

void QStreamDeckAction::setSetting(const QString &key, const QJsonValue &value) {
	settings_[key] = value;
	setSettings(settings_);
}

void QStreamDeckAction::setSettingDefault(const QString &key, const QJsonValue &defaultValue) {
	if(!settings_.contains(key))
		setSetting(key, defaultValue);
}

void QStreamDeckAction::setTitle(const QString &title, int state, QStreamDeckAction::SetTarget target) {
	QJsonObject payload{
		{"title",  title},
		{"target", int(target)},
	};
	if(state != -1)
		payload.insert("state", state);

	sendMessage(+QStreamDeckCommand::setTitle, payload);
}

void QStreamDeckAction::setImage(const QString &data, int state, QStreamDeckAction::SetTarget target) {
	QJsonObject payload{
		{"image",  data},
		{"target", int(target)},
	};
	if(state != -1)
		payload.insert("state", state);

	sendMessage(+QStreamDeckCommand::setImage, payload);
}

void QStreamDeckAction::setImage(const QImage &image, int state, QStreamDeckAction::SetTarget target) {
	setImage(QStreamDeckPlugin::encodeImage(image), state, target);
}

void QStreamDeckAction::setState(int state) {
	sendMessage(+QStreamDeckCommand::setState, QJsonObject{{"state", state}});
}

void QStreamDeckAction::setFeedback(const QJsonObject &data) {
	sendMessage(+QStreamDeckCommand::setFeedback, data);
}

void QStreamDeckAction::setFeedbackLayout(const QString &layout) {
	sendMessage(+QStreamDeckCommand::setFeedbackLayout, {{"layout", layout}});
}

void QStreamDeckAction::sendMessage(const QString &event, const QJsonObject &payload) {
	plugin()->sendMessage(QJsonObject{
		{"event",   event},
		{"context", actionContext()},
		{"payload", payload},
	});
}

void QStreamDeckAction::updatePropertyInspector() {
	QStreamDeckPropertyInspectorBuilder b(this);
	buildPropertyInspector(b);

	propertyInspectorCallback_ = b.buildCallback();
	plugin()->sendMessage(QJsonObject{
		{"event",   +QStreamDeckCommand::sendToPropertyInspector},
		{"context", actionContext_},
		{"payload", QJsonObject{
			{"cmd",     "updateBody"},
			{"content", b.buildHTML()},
		}},
	});
}

void QStreamDeckAction::onEventReceived(const QStreamDeckEvent &e) {
	using ET = QStreamDeckEvent::EventType;

	// Update action state
	state_ = e.payload["state"].toInt();

	switch(e.eventType) {

		case ET::propertyInspectorDidAppear:
			updatePropertyInspector();
			break;

		case ET::sendToPlugin:
			if(propertyInspectorCallback_)
				propertyInspectorCallback_(e);
			break;

		case ET::keyDown:
			isPressed_ = true;
			emit keyDown(e);
			break;

		case ET::keyUp:
			isPressed_ = false;
			emit keyUp(e);
			break;

		case ET::touchTap: {
			const auto tapPosJson = e.payload["tapPos"].toArray();
			emit touchTap(QPoint(tapPosJson[0].toInt(), tapPosJson[1].toInt()), e.payload["hold"].toBool(), e);
			break;
		}

		case ET::dialDown:
			isPressed_ = true;
			emit dialPressed(e);
			break;

		case ET::dialUp:
			isPressed_ = false;
			emit dialReleased(e);
			break;

		case ET::dialRotate:
			emit dialRotated(e.payload["ticks"].toInt(), e);
			break;

		default:
			break;

	}
}

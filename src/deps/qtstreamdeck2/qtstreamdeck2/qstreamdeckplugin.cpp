#include "qstreamdeckplugin.h"

#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QJsonDocument>
#include <QMetaEnum>
#include <QBuffer>
#include <QImage>

#include "qstreamdeckdevice.h"
#include "qstreamdeckaction.h"
#include "qstreamdeckcommand.h"

QString QStreamDeckPlugin::encodeImage(const QImage &image) {
	QBuffer buf;
	buf.open(QIODevice::WriteOnly);
	image.save(&buf, "PNG");
	return "data:image/png;base64," + buf.data().toBase64();
}

QStreamDeckPlugin::QStreamDeckPlugin() {
	connect(&websocket_, &QWebSocket::textMessageReceived, this, &QStreamDeckPlugin::onWebSocketTextMessageReceived);
	connect(&websocket_, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
		qWarning() << "Websocket error: " << websocket_.errorString();
	});
	connect(&websocket_, &QWebSocket::connected, this, [this] {
		qDebug() << "Connected to Stream Deck";
		for(const auto &msg: messageQueue_)
			websocket_.sendTextMessage(msg);

		messageQueue_.clear();
	});

	connect(this, &QStreamDeckPlugin::softwareMessageReceived, this, &QStreamDeckPlugin::onSoftwareMessageReceived);
	connect(this, &QStreamDeckPlugin::eventReceived, this, &QStreamDeckPlugin::onEventReceived);
}

void QStreamDeckPlugin::init(const QString &pluginUID, QCoreApplication &app) {
	// Load up command line parameters
	{
		QCommandLineParser cmdParser;
		cmdParser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
		cmdParser.addHelpOption();

		QCommandLineOption portOption("port", "", "port", "0");
		cmdParser.addOption(portOption);

		QCommandLineOption pluginuuidOption("pluginUUID", "", "pluginUUID", "");
		cmdParser.addOption(pluginuuidOption);

		QCommandLineOption registerEventOption("registerEvent", "", "registerEvent", "");
		cmdParser.addOption(registerEventOption);

		QCommandLineOption infoOption("info", "", "info", "");
		cmdParser.addOption(infoOption);

		cmdParser.process(app);

		port_ = cmdParser.value(portOption).toInt();
		pluginUID_ = pluginUID;
		pluginUUID_ = cmdParser.value(pluginuuidOption);
		registerEvent_ = cmdParser.value(registerEventOption);
		info_ = cmdParser.value(infoOption);
	}

	// Setup global settings
	{
		globalSettingsStorage_.reset(new QSettings(QSettings::UserScope, "Elgato Stream Deck Plugin", pluginUID));
		globalSettings_ = QJsonDocument::fromJson(globalSettingsStorage_->value("globalSettings").toByteArray()).object();
	}

	// Connect to the SW
	websocket_.open(QStringLiteral("ws://localhost:%1").arg(port_));

	// Send the plugin registration message
	sendMessage(QJsonObject{
		{"event", registerEvent_},
		{"uuid",  pluginUUID_}
	});

	// And ask for global settings
	sendMessage(QJsonObject{
		{"event",   +QStreamDeckCommand::getGlobalSettings},
		{"context", pluginUUID_},
	});

	emit initialized();
}

void QStreamDeckPlugin::setGlobalSetting(const QString &key, const QJsonValue &set) {
	globalSettings_[key] = set;
	setGlobalSettings(globalSettings_);
}

void QStreamDeckPlugin::setGlobalSettings(const QJsonObject &set) {
	globalSettings_ = set;
	globalSettingsStorage_->setValue("globalSettings", QJsonDocument(globalSettings_).toJson(QJsonDocument::Compact));

	emit globalSettingsChanged();

	/*sendMessage(QJsonObject{
		{"event",   +QStreamDeckCommand::setGlobalSettings},
		{"context", pluginUUID_},
		{"payload", set},
	});*/
}

void QStreamDeckPlugin::setGlobalSettingDefault(const QString &key, const QJsonValue &defaultValue) {
	if(!globalSettings_.contains(key))
		setGlobalSetting(key, defaultValue);
}

void QStreamDeckPlugin::sendMessage(const QJsonObject &message) {
	//qDebug() << "SND" << message;

	const QString msg = QJsonDocument(message).toJson(QJsonDocument::Compact);
	if(websocket_.isValid())
		websocket_.sendTextMessage(msg);
	else
		messageQueue_ += msg;
}

void QStreamDeckPlugin::onWebSocketTextMessageReceived(const QString &text) {
	const QJsonObject json = QJsonDocument::fromJson(text.toUtf8()).object();
	emit softwareMessageReceived(json);
}

void QStreamDeckPlugin::onSoftwareMessageReceived(const QJsonObject &message) {
	if(const QStreamDeckEvent e = QStreamDeckEvent::fromMessage(message); e.eventType != QStreamDeckEvent::EventType::unknown)
		emit eventReceived(e);
}

void QStreamDeckPlugin::onEventReceived(const QStreamDeckEvent &e) {
	using ET = QStreamDeckEvent::EventType;

	//qDebug() << "RCV" << e.json;
	const QStreamDeckDeviceContext deviceContext = e.json["device"].toString();

	switch(e.eventType) {

		case ET::deviceDidConnect: {
			auto device = std::unique_ptr<QStreamDeckDevice>(createDevice());
			device->init(this, deviceContext, e.json["deviceInfo"].toObject());
			devices_.insert_or_assign(deviceContext, std::move(device));
			return;
		}

		case ET::deviceDidDisconnect: {
			devices_.erase(deviceContext);
			return;
		}

		case ET::didReceiveGlobalSettings: {
			// Transfer from plugin global settings to the QSettings, for compatibility with older versions
			if(globalSettings_.isEmpty() && !e.payload.isEmpty()) {
				setGlobalSettings(e.payload["settings"].toObject());

				// Clear out the SD global settings
				sendMessage(QJsonObject{
					{"event",   +QStreamDeckCommand::setGlobalSettings},
					{"context", pluginUUID_},
					{"payload", QJsonObject{}},
				});
			}
			/*globalSettings_ = e.payload["settings"].toObject();
			emit globalSettingsReceived();*/
			break;
		}

		default:
			break;

	}

	if(!deviceContext.isEmpty()) {
		const auto dit = devices_.find(deviceContext);
		if(dit != devices_.end()) {
			emit dit->second->eventReceived(e);
			return;
		}
		else
			qWarning() << "QtStreamDeck error: Expected device " << deviceContext << "to exist.";
	}

	if(const QStreamDeckActionContext actionContext = e.json["context"].toString(); !actionContext.isEmpty()) {
		QStreamDeckAction *a = actions_.value(actionContext);
		if(a) {
			emit a->eventReceived(e);
			return;
		}
		else
			qWarning() << "QtStreamDeck error: Expected action " << actionContext << "to exist.";
	}
}

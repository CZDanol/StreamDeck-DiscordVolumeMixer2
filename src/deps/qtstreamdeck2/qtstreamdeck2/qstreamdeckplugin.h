#pragma once

#include <QCoreApplication>
#include <QWebSocket>
#include <QJsonObject>
#include <QSettings>

#include "qstreamdeckdeclares.h"
#include "qstreamdeckevent.h"

class QStreamDeckPlugin : public QObject {
Q_OBJECT
	friend class QStreamDeckDevice;
	friend class QStreamDeckAction;

public:
	/// Encodes image to be used by Stream Deck
	static QString encodeImage(const QImage &image);

public:
	QStreamDeckPlugin();
	virtual ~QStreamDeckPlugin() = default;

public:
	/**
 * Initializes the plugin from the command line arguments
 */
	virtual void init(const QString &pluginUID, QCoreApplication &app);

public:
	/// Context handle for the plugin in streamdeck, changes with each run
	inline const QString &pluginUUID() const {
		return pluginUUID_;
	}

	/// Global, persistent plugin UID
	inline const QString &pluginUID() const {
		return pluginUID_;
	}

public:
	/**
 * Returns global settings of the plugin.
 * The settings are not stored through the SD globalSettings API, instead it uses QSettings and registry under the hood.
 * This is because SD globalSettings have to be obtained asynchronously, so they would not be available during plugin startup and initial didAppear events, making things more complicated than they need to be.
 */
	inline const QJsonObject &globalSettings() const {
		return globalSettings_;
	}

	inline const QJsonValue globalSetting(const QString &key) const {
		return globalSettings_[key];
	}

	void setGlobalSetting(const QString &key, const QJsonValue &set);
	void setGlobalSettings(const QJsonObject &set);

	/// If the global setting is not set, sets it to defaultValue
	void setGlobalSettingDefault(const QString &key, const QJsonValue &defaultValue);

public:
	void sendMessage(const QJsonObject &message);

signals:
	void initialized();

	void globalSettingsChanged();

	/**
	 * Emitted whenever any message is received from the software.
	 * The messages are processed internally by this library.
	 * This signal is emitted always, regardless if the message is processed or not.
	 * @param message
	 */
	void softwareMessageReceived(const QJsonObject &message);

	/**
	 * Emited when any known event is received.
	 * When the event has a "device" field, it is routed to the specified device (that can route it to a specific action).
	 * Otherwise, if it only has a "context" field, it is routed directly to a specified action.
	 */
	void eventReceived(const QStreamDeckEvent &e);

	// void globalSettingsReceived();

protected:
	template<typename Action>
	void registerActionType(const QStreamDeckActionUID &actionUID) {
		Q_ASSERT(!actionTypes_.contains(actionUID));
		actionTypes_[actionUID] = []() { return new Action(); };
	}

protected:
	virtual QStreamDeckDevice *createDevice() = 0;

private slots:
	void onWebSocketTextMessageReceived(const QString &text);
	void onSoftwareMessageReceived(const QJsonObject &message);
	void onEventReceived(const QStreamDeckEvent &e);

private:
	int port_ = -1;
	QString pluginUUID_; ///< Context handle for the plugin in streamdeck
	QString pluginUID_; ///< Global, persistent plugin UID
	QString registerEvent_;
	QString info_;

private:
	QWebSocket websocket_;
	QList<QString> messageQueue_; /// Messages to be sent once the socket is connected

private:
	QJsonObject globalSettings_;
	QScopedPointer<QSettings> globalSettingsStorage_;

private:
	std::unordered_map<QStreamDeckDeviceContext, std::unique_ptr<QStreamDeckDevice>> devices_;
	QHash<QStreamDeckActionContext, QStreamDeckAction *> actions_;
	QHash<QStreamDeckActionUID, std::function<QStreamDeckAction *()>> actionTypes_;

};

template<typename Device_>
class QStreamDeckPluginT : public QStreamDeckPlugin {

public:
	using Device = Device_;

protected:
	virtual QStreamDeckDevice *createDevice() final override {
		return new Device();
	}

};
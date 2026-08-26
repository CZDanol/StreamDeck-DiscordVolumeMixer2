#pragma once

#include <QObject>
#include <QJsonObject>

#include "qstreamdeckdeclares.h"
#include "qstreamdeckevent.h"

class QStreamDeckDevice : public QObject {
Q_OBJECT
	friend class QStreamDeckPlugin;

public:
	enum class DeviceType {
		streamDeck,
		streamDeckMini,
		streamDeckXL,
		streamDeckMobile,
		corsairGKeys,
		streamDeckPedal,
		corsairVoyager,
		streamDeckPlus,
	};

	Q_ENUM(DeviceType);

public:
	QStreamDeckDevice();
	virtual ~QStreamDeckDevice();

public:
	inline QStreamDeckPlugin *plugin() {
		return plugin_;
	}

	inline const QStreamDeckDeviceContext &deviceContext() const {
		return deviceContext_;
	}

	inline const QJsonObject &deviceInfo() const {
		return deviceInfo_;
	}

	inline DeviceType deviceType() const {
		return deviceType_;
	}

	/// Returns action grid size
	inline const QPoint &size() const {
		return size_;
	}

public:
	void switchToProfile(const QString &targetProfileName);
	void switchToPreviousProfile();

signals:
	void eventReceived(const QStreamDeckEvent &e);

protected:
	virtual void init(QStreamDeckPlugin *plugin, const QStreamDeckDeviceContext &deviceContext, const QJsonObject &deviceInfo);

private slots:
	void onEventReceived(const QStreamDeckEvent &e);

private:
	QStreamDeckPlugin *plugin_ = nullptr;
	QStreamDeckDeviceContext deviceContext_;
	QJsonObject deviceInfo_;
	DeviceType deviceType_;
	QPoint size_;

private:
	std::unordered_map<QString, std::unique_ptr<QStreamDeckAction>> actions_;

};

template<typename Plugin_>
class QStreamDeckDeviceT : public QStreamDeckDevice {

public:
	using Plugin = Plugin_;

public:
	inline Plugin *plugin() {
		return static_cast<Plugin *>(QStreamDeckDevice::plugin());
	}

};

constexpr inline int operator +(QStreamDeckDevice::DeviceType v) {
	return int(v);
}
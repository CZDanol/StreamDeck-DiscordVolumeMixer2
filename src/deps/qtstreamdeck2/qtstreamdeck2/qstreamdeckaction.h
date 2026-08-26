#pragma once

#include <QObject>
#include <QPoint>

#include "qstreamdeckdeclares.h"
#include "qstreamdeckdevice.h"

class QStreamDeckAction : public QObject {
Q_OBJECT
	friend class QStreamDeckDevice;

public:
	enum class Controller {
		unknown = -1,
		keypad,
		encoder
	};

	Q_ENUM(Controller);

public:
	QStreamDeckAction();
	virtual ~QStreamDeckAction();

public:
	inline QStreamDeckDevice *device() {
		return device_;
	}

	inline QStreamDeckPlugin *plugin() {
		return device_->plugin();
	}

	inline const QString &actionUID() const {
		return actionUID_;
	}

	inline const QStreamDeckActionContext &actionContext() const {
		return actionContext_;
	}

	inline int state() const {
		return state_;
	}

	inline bool isPressed() const {
		return isPressed_;
	}

	inline bool isInMultiAction() const {
		return isInMultiAction_;
	}

	inline const QPoint &coordinates() const {
		return coordinates_;
	}

	inline Controller controller() const {
		return controller_;
	}

public:
	inline const QJsonObject &settings() const {
		return settings_;
	}

	inline QJsonValue setting(const QString &key) const {
		return settings_[key];
	}

	void setSettings(const QJsonObject &set);

	void setSetting(const QString &key, const QJsonValue &value);

	/// If the setting is not set, sets it to defaultValue
	void setSettingDefault(const QString &key, const QJsonValue &defaultValue);

public:
	enum class SetTarget {
		hardwareAndSoftware,
		hardwareOnly,
		softwareOnly
	};

	void setTitle(const QString &title, int state = -1, SetTarget target = SetTarget::hardwareAndSoftware);

	void setImage(const QString &data, int state = -1, SetTarget target = SetTarget::hardwareAndSoftware);
	void setImage(const QImage &image, int state = -1, SetTarget target = SetTarget::hardwareAndSoftware);

	void setState(int state);

	void setFeedback(const QJsonObject &data);
	void setFeedbackLayout(const QString &layout);

public:
	void sendMessage(const QString &event, const QJsonObject &payload);

public slots:
	/**
 * Notifies the system that something about the property inspector has been changed and that the inspector should be rebuilt.
 * Has no effect if a property inspector for the action is not open.
 */
	void updatePropertyInspector();

signals:
	/// Emitted after initialization, when it's safe to access all fields of the action
	void initialized();

	/// Emitted when action settings has been changed (using setSetting/property inspector)
	void settingsChanged();

	void eventReceived(const QStreamDeckEvent &e);
	void keyDown(const QStreamDeckEvent &e);
	void keyUp(const QStreamDeckEvent &e);
	void touchTap(const QPoint &pos, bool hold, const QStreamDeckEvent &e);

	/// Emitted on dialDown
	void dialPressed(const QStreamDeckEvent &e);

	/// Emitted on dialUp
	void dialReleased(const QStreamDeckEvent &e);

	void dialRotated(int delta, const QStreamDeckEvent &e);


protected:
	virtual void init(QStreamDeckDevice *device, const QStreamDeckEvent &appearEvent);

protected:
	/**
	 * This function is called when a property inspector is requested for the action.
	 * This library allows easy property inspector
	 */
	virtual void buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &) {}

private slots:
	void onEventReceived(const QStreamDeckEvent &e);

private:
	QString actionUID_;
	QStreamDeckDevice *device_ = nullptr;
	QStreamDeckActionContext actionContext_;
	QStreamDeckPropertyInspectorCallback propertyInspectorCallback_;
	QJsonObject settings_;
	int state_ = -1;
	bool isPressed_ = false;
	bool isInMultiAction_ = false;
	QPoint coordinates_;
	Controller controller_ = Controller::unknown;

};

template<typename Device_>
class QStreamDeckActionT : public QStreamDeckAction {

public:
	using Device = Device_;

public:
	inline Device *device() {
		return static_cast<Device *>(QStreamDeckAction::device());
	}

	inline typename Device::Plugin *plugin() {
		return device()->plugin();
	}


};

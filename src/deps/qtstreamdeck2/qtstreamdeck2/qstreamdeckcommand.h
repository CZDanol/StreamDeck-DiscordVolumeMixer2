#pragma once

#include <QObject>
#include <QMetaEnum>

namespace QStreamDeckCommand {
	Q_NAMESPACE

	enum Command {
		setSettings,
		getSettings,
		setGlobalSettings,
		getGlobalSettings,
		openUrl,
		logMessage,

		setTitle,
		setImage,
		showAlert,
		showOk,
		setState,
		setFeedback,
		setFeedbackLayout,
		switchToProfile,
		sendToPropertyInspector,

		_cnt
	};

	Q_ENUM_NS(Command);
}

QString operator +(QStreamDeckCommand::Command c);
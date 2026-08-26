#pragma once

#include <QObject>

#include "qdiscordmessage.h"

class QDiscordReply : public QObject {
Q_OBJECT
	friend class QDiscord;

public:
	inline const QString &nonce() const {
		return nonce_;
	}

signals:
	/// Returns when the command was successful
	/// The object gets deleted right after this signal is emitted (deleteLater)
	void success(const QDiscordMessage &msg);

	/// Returns on failure
	/// The object gets deleted right after this signal is emitted (deleteLater)
	void error(const QDiscordMessage &msg);

	/// Returned no matter if the command was success or not
	/// The object gets deleted right after this signal is emitted (deleteLater)
	void finished(const QDiscordMessage &msg);

protected:
	QDiscordReply(const QString &nonce);

	/// Is managed internally in QDiscord
	~QDiscordReply() = default;

private:
	void onFinished(const QDiscordMessage &msg);

private:
	QString nonce_;

};

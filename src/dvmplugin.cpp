#include "dvmplugin.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QDateTime>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>
#include "dvmdevice.h"

#include "action/action_openmixer.h"
#include "action/action_vcminfo.h"
#include "action/action_vcmvolume.h"
#include "action/action_vcmpaging.h"
#include "action/action_back.h"
#include "action/action_deafen.h"
#include "action/action_microphone.h"

#include <cmath>

DVMPlugin::DVMPlugin() {
	registerActionType<Action_OpenMixer>("com.thomast.discordmixer.openmixer");
	registerActionType<Action_VCMInfo>("com.thomast.discordmixer.user");
	registerActionType<Action_VCMVolume>("com.thomast.discordmixer.volumeup");
	registerActionType<Action_VCMVolume>("com.thomast.discordmixer.volumedown");
	registerActionType<Action_VCMPaging>("com.thomast.discordmixer.nextpage");
	registerActionType<Action_VCMPaging>("com.thomast.discordmixer.previouspage");
	registerActionType<Action_Back>("com.thomast.discordmixer.back");
	registerActionType<Action_Microphone>("com.thomast.discordmixer.microphone");
	registerActionType<Action_Deafen>("com.thomast.discordmixer.deafen");

	connect(this, &QStreamDeckPlugin::initialized, this, &DVMPlugin::onInitialized);
	connect(this, &QStreamDeckPlugin::eventReceived, this, &DVMPlugin::onStreamDeckEventReceived);

	connect(&discord, &QDiscord::messageReceived, this, &DVMPlugin::onDiscordMessageReceived);
	connect(&discord, &QDiscord::connected, this, [this] {
		discordReconnectFailures_ = 0;
		discordReconnectTimer_.stop();
	});
	connect(&discord, &QDiscord::disconnected, this, [this] {
		isDiscordConnecting = false;
		currentVoiceChannelID.clear();
		voiceChannelMembers.clear();
		speakingVoiceChannelMembers.clear();
		voiceChannelMemberIxOffset = 0;

		scheduleDiscordReconnect();

		emit buttonsUpdateRequested();
	});

	discordConnectTimeoutTimer_.setSingleShot(true);
	discordConnectTimeoutTimer_.setInterval(2000);

	discordReconnectTimer_.setInterval(2500);
	discordReconnectTimer_.setSingleShot(true);
	discordReconnectTimer_.callOnTimeout(this, &DVMPlugin::connectToDiscord);

	credentialsReconnectTimer_.setInterval(750);
	credentialsReconnectTimer_.setSingleShot(true);
	credentialsReconnectTimer_.callOnTimeout(this, &DVMPlugin::connectToDiscord);

	connect(this, &DVMPlugin::globalSettingsChanged, this, [this] {
		QTimer::singleShot(0, this, [this] {
			const QByteArray newFingerprint = credentialsFingerprint();
			const bool credentialsChanged = !observedCredentialsFingerprint_.isEmpty()
					&& newFingerprint != observedCredentialsFingerprint_;
			observedCredentialsFingerprint_ = newFingerprint;

			if(credentialsChanged) {
				purgeOauthCache();
				rejectedClientID_.clear();
				rejectedClientRetryAfter_ = 0;
				oauthRecoveryClientID_.clear();
				lastConnectionError_.clear();
				if(discord.isConnected())
					discord.disconnect();
			}

			// Property Inspector edits can emit several updates while the user is
			// typing. Debounce them, and never start Discord work before Stream Deck
			// has completed plugin registration.
			if(streamDeckReady_ && !discord.isConnected()) {
				discordReconnectTimer_.stop();
				credentialsReconnectTimer_.start();
			}
		});
	});
}

DVMPlugin::~DVMPlugin() {

}

void DVMPlugin::connectToDiscord() {
	if(!streamDeckReady_ || discord.isConnected() || discord.isProcessing())
		return;

	const QString clientID = globalSetting("client_id").toString().trimmed();
	const QString clientSecret = globalSetting("client_secret").toString().trimmed();
	static const QRegularExpression discordSnowflake(QStringLiteral("^[0-9]{17,20}$"));
	if(clientID.isEmpty() || clientSecret.isEmpty()) {
		lastConnectionError_ = QStringLiteral("SETUP REQUIRED");
		emit buttonsUpdateRequested();
		return;
	}
	if(!discordSnowflake.match(clientID).hasMatch()) {
		lastConnectionError_ = QStringLiteral("INVALID APP ID");
		emit buttonsUpdateRequested();
		return;
	}
	if(!rejectedClientID_.isEmpty() && clientID == rejectedClientID_
			&& QDateTime::currentMSecsSinceEpoch() < rejectedClientRetryAfter_)
		return;

	isDiscordConnecting = true;
	emit buttonsUpdateRequested();
	const bool connected = discord.connect(clientID, clientSecret, QStringLiteral("http://localhost:1337/callback"));
	isDiscordConnecting = false;

	if(connected) {
		rejectedClientID_.clear();
		rejectedClientRetryAfter_ = 0;
		oauthRecoveryClientID_.clear();
		lastConnectionError_.clear();
		// Subscribe to voice channel select event
		discord.sendCommand(+QDiscord::CommandType::subscribe, {}, QJsonObject{
			{"evt", "VOICE_CHANNEL_SELECT"},
		});

		{
			auto r = discord.sendCommand(+QDiscord::CommandType::getVoiceSettings);
			connect(r, &QDiscordReply::success, this, [this](const QDiscordMessage &msg) {
				isMicrophoneMuted = msg.data["mute"].toBool();
				isDeafened = msg.data["deaf"].toBool();
				emit buttonsUpdateRequested();
			});
		}

		updateChannelMembersData();
		discordReconnectTimer_.stop();
		emit buttonsUpdateRequested();
	}
	else {
		emit buttonsUpdateRequested();
		const QString err = discord.connectionError();
		lastConnectionError_ = err;
		if(err == QStringLiteral("BAD CLIENT") || err == QStringLiteral("BAD ID")) {
			// A stale Discord-side authorization can make an otherwise valid app ID
			// fail during the handshake. Clear every local token store and make one
			// automatic clean attempt. Never loop on a deterministic rejection.
			if(oauthRecoveryClientID_ != clientID) {
				oauthRecoveryClientID_ = clientID;
				purgeOauthCache();
				lastConnectionError_ = QStringLiteral("RESETTING AUTH");
				discordReconnectTimer_.start(1500);
				emit buttonsUpdateRequested();
				return;
			}
			rejectedClientID_ = clientID;
			rejectedClientRetryAfter_ = QDateTime::currentMSecsSinceEpoch() + 5 * 60 * 1000;
			discordReconnectTimer_.stop();
			return;
		}
		if(err.contains(QStringLiteral("cancelled"), Qt::CaseInsensitive)
				|| err.contains(QStringLiteral("invalid_scope"), Qt::CaseInsensitive)) {
			discordReconnectTimer_.stop();
			return;
		}
		if(err == QStringLiteral("BAD OAUTH") || err == QStringLiteral("BAD AUTH")
				|| err == QStringLiteral("TOKEN EXP") || err == QStringLiteral("AUTH FAIL")
				|| err == QStringLiteral("NO TOKEN"))
			purgeOauthCache();
		if(err == QStringLiteral("NO ID/SECRET")) {
			discordReconnectTimer_.stop();
			return;
		}
		scheduleDiscordReconnect();
	}
}

QString DVMPlugin::discordStatusText() const {
	if(isDiscordConnecting)
		return QStringLiteral("CONNECTING…");
	if(discord.isConnected())
		return {};

	const QString error = lastConnectionError_.isEmpty()
			? discord.connectionError().trimmed() : lastConnectionError_;
	if(error == QStringLiteral("BAD CLIENT") || error == QStringLiteral("BAD ID"))
		return QStringLiteral("DEAUTHORIZE DISCORD");
	if(error == QStringLiteral("RESETTING AUTH"))
		return QStringLiteral("RESETTING AUTH…");
	if(error == QStringLiteral("NO DISCORD"))
		return QStringLiteral("OPEN DISCORD");
	if(error == QStringLiteral("AUTH DENIED"))
		return QStringLiteral("AUTHORIZE IN DISCORD");
	if(error == QStringLiteral("TOKEN FAIL") || error == QStringLiteral("BAD OAUTH"))
		return QStringLiteral("OAUTH FAILED");
	return error.isEmpty() ? QStringLiteral("DISCORD OFFLINE") : error;
}

QByteArray DVMPlugin::credentialsFingerprint() const {
	const QByteArray credentials = globalSetting("client_id").toString().trimmed().toUtf8()
			+ '\0' + globalSetting("client_secret").toString().trimmed().toUtf8();
	return QCryptographicHash::hash(credentials, QCryptographicHash::Sha256);
}

void DVMPlugin::purgeOauthCache() {
	const QString appDir = QCoreApplication::applicationDirPath();
	const QString oauthPath = !appDir.isEmpty()
			? QDir::cleanPath(QDir(appDir).filePath("../discordOauth.json"))
			: QStringLiteral("discordOauth.json");
	QFile::remove(oauthPath);
	QSettings currentSettings(QSettings::UserScope, "Elgato Stream Deck Plugin", "com.thomast.discordmixer");
	currentSettings.remove("discordOauth");
	QSettings legacySettings(QSettings::UserScope, "Elgato Stream Deck Plugin", "cz.danol.discordmixer");
	legacySettings.remove("discordOauth");
	qDebug() << "Credentials changed; OAuth cache was cleared";
}

void DVMPlugin::migrateLegacySettings() {
	if(globalSetting("legacySettingsMigratedV2").toBool())
		return;

	QJsonObject merged = globalSettings();
	QSettings legacySettings(QSettings::UserScope, "Elgato Stream Deck Plugin", "cz.danol.discordmixer");
	const QJsonObject legacy = QJsonDocument::fromJson(
			legacySettings.value("globalSettings").toByteArray()).object();

	// Only fill missing values. Never overwrite credentials or preferences the
	// user has already entered under the new plugin identity.
	for(auto it = legacy.begin(); it != legacy.end(); ++it) {
		const QJsonValue current = merged.value(it.key());
		if(current.isUndefined() || current.isNull()
				|| (current.isString() && current.toString().trimmed().isEmpty()))
			merged.insert(it.key(), it.value());
	}
	merged.insert("legacySettingsMigratedV2", true);
	setGlobalSettings(merged);

	QSettings currentSettings(QSettings::UserScope, "Elgato Stream Deck Plugin", "com.thomast.discordmixer");
	if(currentSettings.value("discordOauth").toByteArray().isEmpty()) {
		const QByteArray legacyOauth = legacySettings.value("discordOauth").toByteArray();
		const QJsonObject oauth = QJsonDocument::fromJson(legacyOauth).object();
		if(!legacyOauth.isEmpty()
				&& oauth.value("client_id").toString() == merged.value("client_id").toString())
			currentSettings.setValue("discordOauth", legacyOauth);
	}
}

void DVMPlugin::scheduleDiscordReconnect() {
	const int exponent = qMin(discordReconnectFailures_++, 4);
	const int delayMs = qMin(15000, 1500 * (1 << exponent));
	discordReconnectTimer_.start(delayMs);
}

void DVMPlugin::updateChannelMembersData() {
	if(!discord.isConnected())
		return;

	QDiscordReply *r = discord.sendCommand(+QDiscord::CommandType::getSelectedVoiceChannel);
	connect(r, &QDiscordReply::success, this, [this](const QDiscordMessage &msg) {
		updateCurrentVoiceChannel(msg.data["id"].toString());

		// Update voice channel member list
		{
			const auto arr = msg.data["voice_states"].toArray();

			voiceChannelMembers.clear();

			for(const auto &v: arr) {
				const VoiceChannelMember vs = VoiceChannelMember::fromJson(v.toObject());
				if(vs.userID != discord.userID())
					voiceChannelMembers.insert(vs.userID, vs);
			}
		}

		if(voiceChannelMemberIxOffset >= voiceChannelMembers.size())
			voiceChannelMemberIxOffset = 0;

		emit buttonsUpdateRequested();
	});
}

void DVMPlugin::updateSelfVoiceState(const QDiscordMessage &msg) {
	const QJsonObject json = msg.data;

	if(auto v = json["voice_state"]["self_mute"]; !v.isNull())
		isMicrophoneMuted = v.toBool();

	if(auto v = json["voice_state"]["self_deaf"]; !v.isNull())
		isDeafened = v.toBool();
}

void DVMPlugin::adjustVoiceChannelMemberVolume(VoiceChannelMember &vcm, float stepSize, int numSteps) {
	if(!std::isfinite(stepSize) || stepSize <= 0 || numSteps == 0 || !discord.isConnected())
		return;

	float newVolume = vcm.volume + stepSize * numSteps;
	newVolume = qRound(newVolume / stepSize) * stepSize;
	newVolume = qBound(QDiscord::minVoiceVolume, newVolume, QDiscord::maxVoiceVolume);

	if(newVolume != vcm.volume || vcm.isMuted) {
		vcm.volume = newVolume;
		vcm.isMuted = false;

		discord.sendCommand(+QDiscord::CommandType::setUserVoiceSettings, QJsonObject{
			{"user_id", vcm.userID},
			{"volume",  QDiscord::uiToIPCVolume(newVolume)},
			{"mute",    false},
		});
		emit buttonsUpdateRequested();
	}
}

QPixmap DVMPlugin::getUserAvatar(const QString &userId, const QString &avatarHash) {
	if(userId.isEmpty() || avatarHash.isEmpty())
		return {};

	const QString cacheKey = userId + '_' + avatarHash;
	const auto cachedAvatar = avatarCache_.constFind(cacheKey);
	if(cachedAvatar != avatarCache_.cend())
		return cachedAvatar.value();

	if(avatarDownloadsInFlight_.contains(cacheKey)
			|| avatarRetryAfter_.value(cacheKey) > QDateTime::currentMSecsSinceEpoch())
		return {};

	avatarDownloadsInFlight_.insert(cacheKey);
	const QUrl avatarUrl(QStringLiteral("https://cdn.discordapp.com/avatars/%1/%2.png?size=128")
						 .arg(userId, avatarHash));
	QNetworkRequest request(avatarUrl);
	request.setTransferTimeout(10000);
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
			QNetworkRequest::NoLessSafeRedirectPolicy);
	QNetworkReply *reply = avatarNetworkManager_.get(request);
	connect(reply, &QNetworkReply::finished, this, [this, reply, cacheKey] {
		avatarDownloadsInFlight_.remove(cacheKey);
		const qint64 contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
		const bool reasonableSize = contentLength <= 2 * 1024 * 1024;
		const QByteArray avatarData = reasonableSize ? reply->readAll() : QByteArray{};
		const bool networkOk = reply->error() == QNetworkReply::NoError && reasonableSize;
		reply->deleteLater();

		const QPixmap avatar = networkOk ? QPixmap::fromImage(QImage::fromData(avatarData)) : QPixmap{};
		if(avatar.isNull()) {
			qWarning() << "Failed to load Discord avatar" << cacheKey;
			avatarRetryAfter_[cacheKey] = QDateTime::currentMSecsSinceEpoch() + 60000;
			return;
		}

		avatarRetryAfter_.remove(cacheKey);
		avatarCache_.insert(cacheKey, avatar);
		emit buttonsUpdateRequested();
	});

	return {};
}

void DVMPlugin::updateCurrentVoiceChannel(const QString &newVoiceChannel) {
	// If the channel changed, update event subscribtions
	if(newVoiceChannel == currentVoiceChannelID)
		return;

	speakingVoiceChannelMembers.clear();

	static const QStringList events{
		"VOICE_STATE_UPDATE", "VOICE_STATE_CREATE", "VOICE_STATE_DELETE", "SPEAKING_START", "SPEAKING_STOP"
	};
	const auto evf = [&](const QString &cmd) {
		const QJsonObject args{{"channel_id", currentVoiceChannelID}};
		for(const QString &e: events)
			discord.sendCommand(cmd, args, {{"evt", e}});
	};

	if(!currentVoiceChannelID.isEmpty())
		evf(+QDiscord::CommandType::unsubscribe);

	currentVoiceChannelID = newVoiceChannel;

	if(!currentVoiceChannelID.isEmpty())
		evf(+QDiscord::CommandType::subscribe);
}

void DVMPlugin::onDiscordMessageReceived(const QDiscordMessage &msg) {
	using ET = QDiscordMessage::EventType;

	switch(msg.event) {

		// Voice channel changed
		case ET::voiceChannelSelect:
			updateCurrentVoiceChannel(msg.data["channel_id"].toString());
			updateChannelMembersData();
			break;

		case ET::voiceStateCreate: {
			const auto m = VoiceChannelMember::fromJson(msg.data);
			if(m.userID == discord.userID())
				updateSelfVoiceState(msg);

			else
				voiceChannelMembers.insert(m.userID, m);

			break;
		}

		case ET::voiceStateUpdate: {
			const auto m = VoiceChannelMember::fromJson(msg.data);
			if(m.userID == discord.userID())
				updateSelfVoiceState(msg);

			else
				voiceChannelMembers.insert(m.userID, m);

			break;
		}

		case ET::voiceStateDelete: {
			const auto voiceData = VoiceChannelMember::fromJson(msg.data);
			voiceChannelMembers.remove(voiceData.userID);
			speakingVoiceChannelMembers.remove(voiceData.userID);
			if(voiceChannelMemberIxOffset >= voiceChannelMembers.size())
				voiceChannelMemberIxOffset = 0;

			/*
			 * Bug workaround - when voice state delete reports the current user, it possibly means that the user has been moved by admin to another voice channel, which does not trigger the VOICE_CHANNEL_SELECT event.
			 * So in this case, we requery everything.
			 * */
			if(voiceData.userID == discord.userID())
				updateChannelMembersData();

			break;
		}

		case ET::speakingStart:
			speakingVoiceChannelMembers.insert(msg.data["user_id"].toString());
			break;

		case ET::speakingStop:
			speakingVoiceChannelMembers.remove(msg.data["user_id"].toString());
			break;

		case ET::voiceSettingsUpdate:
			isMicrophoneMuted = msg.data["mute"].toBool();
			isDeafened = msg.data["deaf"].toBool();
			emit buttonsUpdateRequested();
			break;

		default:
			return;

	}

	emit buttonsUpdateRequested();
}

void DVMPlugin::onInitialized() {
	migrateLegacySettings();
	observedCredentialsFingerprint_ = credentialsFingerprint();
	setGlobalSettingDefault("voiceChannelVolumeButtonStep", 5);
	setGlobalSettingDefault("voiceChannelVolumeEncoderStep", 5);
}

void DVMPlugin::onStreamDeckEventReceived(const QStreamDeckEvent &e) {
	using ET = QStreamDeckEvent::EventType;

	// deviceDidConnect proves that WebSocket registration with Stream Deck has
	// completed. Only then is it safe to begin potentially blocking Discord IPC
	// and OAuth work during a simultaneous PC/Discord/Stream Deck startup.
	if(e.eventType == ET::deviceDidConnect && !discord.isConnected()
			&& !discord.isProcessing() && !discordReconnectTimer_.isActive()) {
		streamDeckReady_ = true;
		discordReconnectTimer_.start(250);
	}

	// Try connecting to discord whenever any button is pressed
	if(!discord.isConnected() && !discordConnectTimeoutTimer_.isActive() && (e.eventType == ET::touchTap || e.eventType == ET::keyDown || e.eventType == ET::dialDown || e.eventType == ET::dialUp || e.eventType == ET::dialRotate)) {
		const qint64 now = QDateTime::currentMSecsSinceEpoch();
		// A deliberate key press is an explicit retry after the user has followed
		// the Deauthorize/Authorize instruction. Permit it immediately, but throttle
		// repeated presses so a held key cannot create an RPC reconnect loop.
		if(!rejectedClientID_.isEmpty() && now - lastManualReconnectAttempt_ >= 5000) {
			rejectedClientRetryAfter_ = 0;
			lastManualReconnectAttempt_ = now;
		}
		discordConnectTimeoutTimer_.start();
		discordReconnectTimer_.stop();
		connectToDiscord();
	}
}

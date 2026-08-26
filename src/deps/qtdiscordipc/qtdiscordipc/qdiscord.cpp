#include "qdiscord.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QJsonDocument>
#include <QMetaEnum>
#include <QNetworkReply>
#include <QRandomGenerator64>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>
#include <QUrlQuery>
#include <QtEndian>

#include <cmath>
#include <utility>

namespace {
constexpr quint32 handshakeOpcode = 0;
constexpr quint32 frameOpcode = 1;
constexpr quint32 closeOpcode = 2;
constexpr quint32 pingOpcode = 3;
constexpr quint32 pongOpcode = 4;
constexpr qsizetype headerSize = 8;
constexpr quint32 maximumPayloadSize = 16 * 1024 * 1024;
constexpr int replyTimeoutMs = 15000;
constexpr int pipeConnectTimeoutMs = 250;
// Discord can take several seconds to answer while its desktop client is still
// starting. This wait is event-driven, so a generous timeout does not freeze
// Stream Deck or its Property Inspector.
constexpr int handshakeTimeoutMs = 10000;

QJsonObject errorMessage(const QString &message) {
    return QJsonObject{
        {"evt", "ERROR"},
        {"data", QJsonObject{{"code", -1}, {"message", message}}},
    };
}
}

double QDiscord::ipcToUIVolume(double v) {
    if(v <= 0)
        return 0;
    if(v <= 100)
        return 17.362 * std::log(v) + 20.054;
    return 144.86 * std::log(v) - 567.21;
}

double QDiscord::uiToIPCVolume(double v) {
    if(v <= 0)
        return 0;
    if(v <= 100)
        return std::exp((v - 20.054) / 17.362);
    return std::exp((v + 567.21) / 144.86);
}

QDiscord::QDiscord() {
    QObject::connect(&socket_, &QLocalSocket::errorOccurred, this, [this](QLocalSocket::LocalSocketError err) {
        qWarning() << "QDiscord socket error:" << static_cast<int>(err) << socket_.errorString();
    });

    QObject::connect(&socket_, &QLocalSocket::disconnected, this, [this] {
        qDebug() << "Discord IPC disconnected";
        if(connectionError_.isEmpty())
            connectionError_ = "DISCONNECTED";

        const bool wasConnected = isConnected_;
        isConnected_ = false;
        userID_.clear();
        receiveBuffer_.clear();
        failPendingReplies(connectionError_);
        if(wasConnected)
            emit disconnected();
    });

    QObject::connect(&socket_, &QLocalSocket::readyRead, this, [this] {
        if(blockingRead_)
            return;
        readAndProcessMessages();
    });
}

QDiscord::~QDiscord() {
    socket_.abort();
    for(QDiscordReply *reply: std::as_const(pendingReplies_))
        delete reply;
    pendingReplies_.clear();
}

bool QDiscord::connect(const QString &clientID, const QString &clientSecret, const QString &redirectUri) {
    if(processing_ > 0)
        return false;

    connectionError_.clear();
    processing_++;
    const bool connectionSucceeded = [&]() {
        if(clientID.trimmed().isEmpty() || clientSecret.trimmed().isEmpty()) {
            qWarning() << "Missing Discord client ID or secret";
            connectionError_ = "NO ID/SECRET";
            return false;
        }

        socket_.abort();
        receiveBuffer_.clear();

        static const QStringList scopes{"rpc", "identify"};

        // Try each IPC endpoint (0–9). For each one that accepts a TCP connection,
        // attempt the full handshake. If the handshake fails (timeout, close frame,
        // or unexpected response), disconnect and try the next pipe. This handles
        // stale/orphaned pipes left behind by Discord restarts.
        bool handshakeOk = false;
		QString handshakeError;
        for(int i = 0; i < 10; i++) {
            socket_.abort();
            receiveBuffer_.clear();
            connectionError_.clear();

            socket_.connectToServer("discord-ipc-" + QString::number(i), QIODevice::ReadWrite);
            if(socket_.state() != QLocalSocket::ConnectedState) {
                // Keep Qt's event loop alive while Discord's named pipe is being
                // located. Stream Deck must continue receiving WebSocket events
                // or it may mark the plugin as unresponsive.
                QEventLoop connectionLoop;
                QTimer connectionTimeout;
                connectionTimeout.setSingleShot(true);
                QObject::connect(&socket_, &QLocalSocket::connected,
                                 &connectionLoop, &QEventLoop::quit);
                QObject::connect(&socket_, &QLocalSocket::errorOccurred,
                                 &connectionLoop, &QEventLoop::quit);
                QObject::connect(&connectionTimeout, &QTimer::timeout,
                                 &connectionLoop, &QEventLoop::quit);
                connectionTimeout.start(pipeConnectTimeoutMs);
                connectionLoop.exec();
            }
            if(socket_.state() != QLocalSocket::ConnectedState) {
                socket_.abort();
                continue;
            }
            qDebug() << "Connected to Discord IPC pipe" << i << "- attempting handshake";

            sendMessage(QJsonObject{{"v", 1}, {"client_id", clientID}}, handshakeOpcode);
            const auto ready = readMessage(handshakeTimeoutMs);
            if(!ready) {
				if(handshakeError.isEmpty() && !connectionError_.isEmpty())
					handshakeError = connectionError_;
                qWarning() << "Discord IPC pipe" << i << "handshake failed:"
                           << (connectionError_.isEmpty() ? "timeout/disconnect" : connectionError_);
                socket_.abort();
                // These are deterministic application configuration failures.
                // Trying the remaining pipes only delays the same answer.
                if(connectionError_ == QStringLiteral("BAD CLIENT")
                        || connectionError_ == QStringLiteral("BAD ID"))
                    break;
                continue;
            }
            if(ready->opcode != frameOpcode || ready->json["cmd"] != "DISPATCH" || ready->json["evt"] != "READY") {
                qWarning() << "Discord IPC pipe" << i << "unexpected handshake response:" << ready->json;
                socket_.abort();
                continue;
            }

            qDebug() << "Discord IPC handshake successful on pipe" << i;
            cdn_ = ready->data["config"].toObject()["cdn_host"].toString();
            handshakeOk = true;
            break;
        }

        if(!handshakeOk) {
			if(!handshakeError.isEmpty())
				connectionError_ = handshakeError;
			else if(connectionError_.isEmpty())
                connectionError_ = "NO DISCORD";
            qWarning() << "All Discord IPC endpoints failed:" << connectionError_;
            return false;
        }

        // Robust absolute file path for OAuth token cache (AppData plugin directory)
        static const auto getOauthFilePath = []() -> QString {
            const QString appDir = QCoreApplication::applicationDirPath();
            if(!appDir.isEmpty()) {
                const QString candidate = QDir::cleanPath(QDir(appDir).filePath("../discordOauth.json"));
                return candidate;
            }
            return QDir::cleanPath(QDir::current().filePath("discordOauth.json"));
        };

        const QString oauthPath = getOauthFilePath();
        QJsonObject oauthData;

        // 1. Load from AppData JSON file
        QFile oauthFile(oauthPath);
        if(oauthFile.open(QIODevice::ReadOnly)) {
            QJsonParseError parseError;
            oauthData = QJsonDocument::fromJson(oauthFile.readAll(), &parseError).object();
            if(parseError.error != QJsonParseError::NoError)
                qWarning() << "Ignoring invalid OAuth token cache file:" << parseError.errorString();
            oauthFile.close();
        }

        // 2. Backup load from the current Registry settings if the file is
        // missing or empty. Legacy migration is handled once by DVMPlugin;
        // reading the old key here could resurrect a token that was deliberately
        // invalidated after a Client Secret reset or Discord deauthorization.
        if(oauthData.isEmpty() || oauthData["client_id"].toString() != clientID) {
            QSettings regSettings(QSettings::UserScope, "Elgato Stream Deck Plugin", "com.thomast.discordmixer");
            const QByteArray regBytes = regSettings.value("discordOauth").toByteArray();
            if(!regBytes.isEmpty()) {
                QJsonObject regObj = QJsonDocument::fromJson(regBytes).object();
                if(regObj["client_id"].toString() == clientID)
                    oauthData = regObj;
            }
        }

        if(oauthData["client_id"].toString() != clientID)
            oauthData = {};

        const auto saveOauthData = [&]() -> bool {
            oauthData["client_id"] = clientID;
            const QByteArray jsonBytes = QJsonDocument(oauthData).toJson(QJsonDocument::Compact);

            // Save to Registry backup
            QSettings regSettings(QSettings::UserScope, "Elgato Stream Deck Plugin", "com.thomast.discordmixer");
            regSettings.setValue("discordOauth", jsonBytes);

            // Save to absolute AppData file path
            QFile file(oauthPath);
            if(file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                file.write(jsonBytes);
                file.flush();
                file.close();
                qDebug() << "OAuth token cache saved successfully to" << oauthPath;
                return true;
            }
            qWarning() << "Unable to save OAuth token cache file to" << oauthPath << ":" << file.errorString();
            return true; // Still true because Registry backup succeeded
        };

        const auto postTokenRequest = [&](const QUrlQuery &form, const char *stage) -> std::optional<QJsonObject> {
            QNetworkAccessManager manager;
            QNetworkRequest request(QUrl("https://discord.com/api/oauth2/token"));
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
            request.setRawHeader("User-Agent", "StreamDeck-DiscordVolumeMixer/2.0.3");

            QNetworkReply *reply = manager.post(request, form.toString(QUrl::FullyEncoded).toUtf8());
            QEventLoop loop;
            QTimer timeout;
            timeout.setSingleShot(true);
            timeout.start(replyTimeoutMs);
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            QObject::connect(&timeout, &QTimer::timeout, reply, &QNetworkReply::abort);
            QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
            loop.exec();

            const auto networkError = reply->error();
            const QString networkErrorText = reply->errorString();
            const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const QByteArray body = reply->readAll();
            reply->deleteLater();

            QJsonParseError parseError;
            const QJsonObject response = QJsonDocument::fromJson(body, &parseError).object();
            if(networkError != QNetworkReply::NoError || httpStatus < 200 || httpStatus >= 300 ||
               parseError.error != QJsonParseError::NoError) {
                const QString apiMessage = response["error_description"].toString(response["message"].toString());
                qWarning() << "Discord OAuth" << stage << "failed; HTTP" << httpStatus
                           << networkErrorText << apiMessage;
                return std::nullopt;
            }
            return response;
        };

        const auto loadIdentityFromAuth = [&](const QJsonObject &message) {
            userID_ = message["data"].toObject()["user"].toObject()["id"].toString();
        };

        // 1. FAST PATH: Authenticate immediately with existing cached access_token (takes ~5ms over IPC)
        if(!oauthData["access_token"].toString().isEmpty()) {
            sendMessage(QJsonObject{
                {"cmd", +CommandType::authenticate},
                {"nonce", "auth_0"},
                {"args", QJsonObject{{"access_token", oauthData["access_token"].toString()}}},
            });
            const auto auth = readMessage(5000);
            if(auth && auth->json["cmd"] == "AUTHENTICATE" && auth->json["evt"] != "ERROR") {
                qDebug() << "Authenticated instantly with cached Discord token";
                loadIdentityFromAuth(auth->json);
                return !userID_.isEmpty();
            }
            qDebug() << "Cached access_token was expired or rejected, attempting refresh";
            oauthData["access_token"] = "";
        }

        // 2. REFRESH PATH: If access_token failed, try refresh_token in the background without popup
        if(!oauthData["refresh_token"].toString().isEmpty()) {
            const QString previousRefreshToken = oauthData["refresh_token"].toString();
            const QUrlQuery form{
                {"client_id", clientID},
                {"client_secret", clientSecret},
                {"refresh_token", previousRefreshToken},
                {"scope", scopes.join(' ')},
                {"grant_type", "refresh_token"},
            };
            if(auto refreshed = postTokenRequest(form, "refresh")) {
                oauthData = *refreshed;
                if(oauthData["refresh_token"].toString().isEmpty())
                    oauthData["refresh_token"] = previousRefreshToken;
                saveOauthData();

                if(!oauthData["access_token"].toString().isEmpty()) {
                    sendMessage(QJsonObject{
                        {"cmd", +CommandType::authenticate},
                        {"nonce", "auth_refresh"},
                        {"args", QJsonObject{{"access_token", oauthData["access_token"].toString()}}},
                    });
                    const auto auth = readMessage(5000);
                    if(auth && auth->json["cmd"] == "AUTHENTICATE" && auth->json["evt"] != "ERROR") {
                        qDebug() << "Authenticated successfully with refreshed Discord token";
                        loadIdentityFromAuth(auth->json);
                        return !userID_.isEmpty();
                    }
                }
            }
            oauthData = {};
        }

        sendMessage(QJsonObject{
            {"cmd", +CommandType::authorize},
            {"nonce", "auth_1"},
            {"args", QJsonObject{{"client_id", clientID}, {"scopes", QJsonArray::fromStringList(scopes)}}},
        });
        const auto authorization = readMessage(30000);
        if(!authorization || authorization->json["cmd"] != "AUTHORIZE" || authorization->json["evt"] == "ERROR") {
            if(authorization)
                qWarning() << "Discord authorization failed" << authorization->json;
            if(connectionError_.isEmpty())
                connectionError_ = "AUTH DENIED";
            return false;
        }
        const QString authCode = authorization->data["code"].toString();
        if(authCode.isEmpty()) {
            connectionError_ = "AUTH DENIED";
            return false;
        }

        const QUrlQuery exchangeForm{
            {"client_id", clientID},
            {"client_secret", clientSecret},
            {"code", authCode},
            {"scope", scopes.join(' ')},
            {"grant_type", "authorization_code"},
            {"redirect_uri", redirectUri},
        };
        auto exchanged = postTokenRequest(exchangeForm, "authorization-code exchange");
        if(!exchanged) {
            connectionError_ = "TOKEN FAIL";
            return false;
        }
        oauthData = *exchanged;
        if(oauthData["access_token"].toString().isEmpty()) {
            qWarning() << "Discord OAuth response did not contain an access token";
            connectionError_ = "NO TOKEN";
            return false;
        }
        saveOauthData();

        sendMessage(QJsonObject{
            {"cmd", +CommandType::authenticate},
            {"nonce", "auth_2"},
            {"args", QJsonObject{{"access_token", oauthData["access_token"].toString()}}},
        });
        const auto auth = readMessage();
        if(!auth || auth->json["cmd"] != "AUTHENTICATE" || auth->json["evt"] == "ERROR") {
            if(auth)
                qWarning() << "Discord authentication failed" << auth->json;
            if(connectionError_.isEmpty())
                connectionError_ = "AUTH FAIL";
            return false;
        }
        loadIdentityFromAuth(auth->json);
        if(userID_.isEmpty()) {
            connectionError_ = "AUTH FAIL";
            return false;
        }

        qDebug() << "Discord connection successful";
        return true;
    }();

    if(!connectionSucceeded) {
        socket_.abort();
        userID_.clear();
    }
    else {
        isConnected_ = true;
        connectionError_.clear();
        emit connected();
    }

    processing_--;
    if(connectionSucceeded)
        QTimer::singleShot(0, this, &QDiscord::readAndProcessMessages);
    return connectionSucceeded;
}

void QDiscord::disconnect() {
    const bool wasConnected = isConnected_;
    isConnected_ = false;
    userID_.clear();
    receiveBuffer_.clear();
    failPendingReplies("DISCONNECTED");
    socket_.abort();
    if(wasConnected)
        emit disconnected();
}

QDiscordReply *QDiscord::sendCommand(const QString &command, const QJsonObject &args, const QJsonObject &msgOverrides) {
    const QString nonce = QStringLiteral("%1:%2").arg(QString::number(nonceCounter_++), QString::number(QRandomGenerator64::global()->generate()));
    QDiscordReply *reply = new QDiscordReply(nonce);
    pendingReplies_.insert(nonce, reply);

    if(!isConnected_) {
        QTimer::singleShot(0, this, [this, nonce] {
            if(QDiscordReply *pending = pendingReplies_.take(nonce)) {
                emit pending->finished(QDiscordMessage::fromJson(errorMessage("Discord is not connected"), frameOpcode));
                pending->deleteLater();
            }
        });
        return reply;
    }

    QJsonObject message{{"cmd", command}, {"args", args}, {"nonce", nonce}};
    for(auto it = msgOverrides.begin(); it != msgOverrides.end(); ++it)
        message[it.key()] = it.value();
    sendMessage(message);

    QTimer::singleShot(replyTimeoutMs, this, [this, nonce] {
        if(QDiscordReply *pending = pendingReplies_.take(nonce)) {
            emit pending->finished(QDiscordMessage::fromJson(errorMessage("Discord command timed out"), frameOpcode));
            pending->deleteLater();
        }
    });
    return reply;
}

QImage QDiscord::getUserAvatar(const QString &userId, const QString &avatarId) {
    if(userId.isEmpty() || avatarId.isEmpty())
        return {};

    const QString cacheKey = userId + ':' + avatarId;
    if(QImage *image = avatarsCache_.object(cacheKey))
        return *image;
    if(avatarsInFlight_.contains(cacheKey))
        return {};
    if(avatarRetryAfter_.value(cacheKey) > QDateTime::currentMSecsSinceEpoch())
        return {};

    avatarsInFlight_.insert(cacheKey);
    const QString cdnHost = cdn_.isEmpty() ? QStringLiteral("cdn.discordapp.com") : cdn_;
    const QUrl url(QStringLiteral("https://%1/avatars/%2/%3.png?size=128").arg(cdnHost, userId, avatarId));
    QNetworkReply *reply = netMgr_.get(QNetworkRequest(url));
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, avatarId, cacheKey] {
        avatarsInFlight_.remove(cacheKey);
        const QByteArray data = reply->readAll();
        const bool networkOk = reply->error() == QNetworkReply::NoError;
        reply->deleteLater();

        const QImage image = networkOk ? QImage::fromData(data) : QImage{};
        if(image.isNull()) {
            qWarning() << "Failed to load Discord avatar" << cacheKey;
            avatarRetryAfter_[cacheKey] = QDateTime::currentMSecsSinceEpoch() + 60000;
            return;
        }
        avatarRetryAfter_.remove(cacheKey);
        avatarsCache_.insert(cacheKey, new QImage(image));
        emit avatarReady(avatarId, image);
    });
    return {};
}

std::optional<QDiscordMessage> QDiscord::readMessage(int timeoutMs) {
    for(;;) {
        const auto frame = readFrame(timeoutMs);
        if(!frame)
            return std::nullopt;

        if(frame->opcode == pingOpcode) {
            sendFrame(frame->payload, pongOpcode);
            continue;
        }
        if(frame->opcode == pongOpcode)
            continue;

        QJsonParseError parseError;
        const QJsonObject json = QJsonDocument::fromJson(frame->payload, &parseError).object();
        if(frame->opcode == closeOpcode) {
            setCloseError(json);
            return std::nullopt;
        }
        if(frame->opcode != frameOpcode || parseError.error != QJsonParseError::NoError) {
            qWarning() << "Invalid Discord IPC frame; opcode" << frame->opcode << parseError.errorString();
            connectionError_ = "ERR IPC";
            return std::nullopt;
        }
        qDebug() << "Discord IPC receive" << json["cmd"].toString() << json["evt"].toString();
        return QDiscordMessage::fromJson(json, static_cast<int>(frame->opcode));
    }
}

std::optional<QDiscord::IpcFrame> QDiscord::readFrame(int timeoutMs) {
    QElapsedTimer elapsed;
    elapsed.start();
    blockingRead_++;

    for(;;) {
        receiveBuffer_.append(socket_.readAll());
        if(auto frame = takeBufferedFrame()) {
            blockingRead_--;
            return frame;
        }
        if(socket_.state() != QLocalSocket::ConnectedState || elapsed.elapsed() >= timeoutMs) {
            blockingRead_--;
            return std::nullopt;
        }
        const int remaining = qMax(1, timeoutMs - static_cast<int>(elapsed.elapsed()));

        // A nested Qt event loop keeps the Stream Deck WebSocket, timers and UI
        // responsive while Discord displays its authorization dialog. The old
        // waitForReadyRead() blocked the whole plugin for up to 30 seconds.
        QEventLoop readLoop;
        QTimer readTimeout;
        readTimeout.setSingleShot(true);
        QObject::connect(&socket_, &QLocalSocket::readyRead,
                         &readLoop, &QEventLoop::quit);
        QObject::connect(&socket_, &QLocalSocket::disconnected,
                         &readLoop, &QEventLoop::quit);
        QObject::connect(&readTimeout, &QTimer::timeout,
                         &readLoop, &QEventLoop::quit);
        readTimeout.start(remaining);
        readLoop.exec();
    }
}

std::optional<QDiscord::IpcFrame> QDiscord::takeBufferedFrame() {
    if(receiveBuffer_.size() < headerSize)
        return std::nullopt;

    const auto *raw = reinterpret_cast<const uchar *>(receiveBuffer_.constData());
    const quint32 opcode = qFromLittleEndian<quint32>(raw);
    const quint32 length = qFromLittleEndian<quint32>(raw + sizeof(quint32));
    if(length > maximumPayloadSize) {
        qWarning() << "Discord IPC payload exceeds safety limit:" << length;
        connectionError_ = "ERR IPC";
        receiveBuffer_.clear();
        socket_.abort();
        return std::nullopt;
    }
    if(receiveBuffer_.size() < headerSize + static_cast<qsizetype>(length))
        return std::nullopt;

    IpcFrame frame{opcode, receiveBuffer_.mid(headerSize, length)};
    receiveBuffer_.remove(0, headerSize + length);
    return frame;
}

void QDiscord::sendMessage(const QJsonObject &packet, int opCode) {
    const QByteArray payload = QJsonDocument(packet).toJson(QJsonDocument::Compact);
    qDebug() << "Discord IPC send" << packet["cmd"].toString() << packet["evt"].toString();
    sendFrame(payload, static_cast<quint32>(opCode));
}

void QDiscord::sendFrame(const QByteArray &payload, quint32 opCode) {
    if(socket_.state() != QLocalSocket::ConnectedState)
        return;

    QByteArray header(headerSize, '\0');
    auto *raw = reinterpret_cast<uchar *>(header.data());
    qToLittleEndian(opCode, raw);
    qToLittleEndian(static_cast<quint32>(payload.size()), raw + sizeof(quint32));
    socket_.write(header);
    socket_.write(payload);
    socket_.flush();
}

void QDiscord::processMessage(const QDiscordMessage &msg) {
    if(QDiscordReply *reply = pendingReplies_.take(msg.nonce)) {
        emit reply->finished(msg);
        reply->deleteLater();
        return;
    }
    emit messageReceived(msg);
}

void QDiscord::readAndProcessMessages() {
    if(blockingRead_)
        return;

    receiveBuffer_.append(socket_.readAll());
    while(auto frame = takeBufferedFrame())
        processFrame(*frame);
}

void QDiscord::processFrame(const IpcFrame &frame) {
    if(frame.opcode == pingOpcode) {
        sendFrame(frame.payload, pongOpcode);
        return;
    }
    if(frame.opcode == pongOpcode)
        return;

    QJsonParseError parseError;
    const QJsonObject json = QJsonDocument::fromJson(frame.payload, &parseError).object();
    if(frame.opcode == closeOpcode) {
        setCloseError(json);
        socket_.abort();
        return;
    }
    if(frame.opcode != frameOpcode || parseError.error != QJsonParseError::NoError) {
        qWarning() << "Ignoring invalid Discord IPC frame; opcode" << frame.opcode << parseError.errorString();
        return;
    }
    processMessage(QDiscordMessage::fromJson(json, static_cast<int>(frame.opcode)));
}

void QDiscord::failPendingReplies(const QString &message) {
    const auto replies = pendingReplies_;
    pendingReplies_.clear();
    const QDiscordMessage failure = QDiscordMessage::fromJson(errorMessage(message), frameOpcode);
    for(QDiscordReply *reply: replies) {
        emit reply->finished(failure);
        reply->deleteLater();
    }
}

void QDiscord::setCloseError(const QJsonObject &payload) {
    const int code = payload["code"].toInt(payload["data"].toObject()["code"].toInt());
    const QString message = payload["message"].toString(payload["data"].toObject()["message"].toString());
    qWarning() << "Discord closed the IPC connection; code" << code << "message" << message;

    // Translate common Discord RPC close codes to human-readable error strings
    static const QHash<int, QString> closeCodeDescriptions = {
        {4000, "BAD CLIENT"},    // Invalid client ID
        {4002, "BAD OAUTH"},     // Invalid OAuth2 token
        {4003, "NO AUTH"},       // Not authenticated
        {4004, "BAD AUTH"},      // Authentication failed
        {4006, "BAD NONCE"},     // Invalid nonce
        {4007, "BAD ID"},        // Invalid client ID
        {4008, "BAD ORIGIN"},    // Invalid origin / redirect URI not configured
        {4009, "TOKEN EXP"},     // Token expired or invalid
        {4010, "BAD ENCODING"}, // Invalid encoding
    };

    if(code > 0) {
        connectionError_ = closeCodeDescriptions.value(code, QStringLiteral("RPC %1").arg(code));
    } else {
        connectionError_ = "CLOSED";
    }
}

QString operator +(QDiscord::CommandType ct) {
    static const QHash<int, QString> names = [] {
        const auto metaEnum = QMetaEnum::fromType<QDiscord::CommandType>();
        QHash<int, QString> result;
        result.reserve(metaEnum.keyCount());
        const QRegularExpression regex("([A-Z])");
        for(int i = 0; i < metaEnum.keyCount(); i++) {
            QString name = metaEnum.key(i);
            name.replace(regex, "_\\1");
            result.insert(metaEnum.value(i), name.toUpper());
        }
        return result;
    }();
    return names.value(static_cast<int>(ct));
}

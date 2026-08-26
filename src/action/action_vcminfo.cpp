#include "action_vcminfo.h"

#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QLinearGradient>
#include <QFontMetricsF>

#include <qtstreamdeck2/qstreamdeckpropertyinspectorbuilder.h>

#include "dvmplugin.h"

Action_VCMInfo::Action_VCMInfo() {
	connect(this, &QStreamDeckAction::keyDown, this, &Action_VCMInfo::onPressed);
	connect(this, &QStreamDeckAction::keyUp, this, &Action_VCMInfo::onReleased);

	connect(this, &QStreamDeckAction::dialPressed, this, &Action_VCMInfo::onPressed);
	connect(this, &QStreamDeckAction::dialRotated, this, &Action_VCMInfo::onRotated);
	connect(this, &QStreamDeckAction::touchTap, this, &Action_VCMInfo::onTapped);
}

void Action_VCMInfo::update() {
	if(controller() == Controller::keypad)
		update_button();
	else
		update_encoder();
}

static QString formatButtonNick(const QString &rawNick) {
	const QString nick = rawNick.trimmed();
	if(nick.length() <= 8)
		return nick;

	// If contains space, e.g. "John Smith" -> "John S."
	const int spaceIdx = nick.indexOf(' ');
	if(spaceIdx > 0 && spaceIdx <= 7) {
		if(spaceIdx + 2 <= nick.length() && !nick.at(spaceIdx + 1).isSpace())
			return nick.left(spaceIdx + 2) + ".";
		return nick.left(spaceIdx);
	}

	// Truncate with clean ellipsis (7 chars + ellipsis = 8 chars total)
	return nick.left(7) + QString::fromUtf8("…");
}

namespace {

constexpr int buttonImageSize = 144;

void drawAvatarPlaceholder(QPainter &painter, const QRectF &avatarRect) {
	static const QImage placeholder("icons/icons8_user_72px.png");
	if(!placeholder.isNull()) {
		const QRectF target = avatarRect.adjusted(12, 12, -12, -12);
		painter.setOpacity(0.72);
		painter.drawImage(target, placeholder);
		painter.setOpacity(1.0);
		return;
	}

	// Asset-independent fallback silhouette.
	painter.setBrush(QColor("#9AA0A6"));
	painter.setPen(Qt::NoPen);
	const QPointF center = avatarRect.center();
	painter.drawEllipse(QPointF(center.x(), center.y() - 12), 11, 11);
	QPainterPath shoulders;
	shoulders.addEllipse(QRectF(center.x() - 24, center.y() + 3, 48, 32));
	painter.drawPath(shoulders);
}

void drawMutedBadge(QPainter &painter, const QRectF &avatarRect) {
	const QRectF badgeRect(avatarRect.right() - 25, avatarRect.bottom() - 28, 32, 32);
	painter.setPen(QPen(QColor("#0A0B0C"), 3));
	painter.setBrush(QColor("#ED4245"));
	painter.drawEllipse(badgeRect);

	// Small microphone plus slash; kept deliberately bold for 72 px devices.
	painter.setBrush(Qt::NoBrush);
	painter.setPen(QPen(Qt::white, 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
	const QPointF center = badgeRect.center();
	painter.drawRoundedRect(QRectF(center.x() - 3, center.y() - 8, 6, 11), 3, 3);
	painter.drawArc(QRectF(center.x() - 6, center.y() - 5, 12, 12), 195 * 16, 150 * 16);
	painter.drawLine(QPointF(center.x(), center.y() + 7), QPointF(center.x(), center.y() + 10));
	painter.drawLine(QPointF(center.x() - 5, center.y() + 10), QPointF(center.x() + 5, center.y() + 10));
	painter.setPen(QPen(Qt::white, 3, Qt::SolidLine, Qt::RoundCap));
	painter.drawLine(badgeRect.topLeft() + QPointF(7, 6),
					 badgeRect.bottomRight() - QPointF(7, 6));
}

QImage renderButtonTile(const VoiceChannelMember *member, const QImage &avatar,
						bool isSpeaking, const QString &message) {
	QImage tile(buttonImageSize, buttonImageSize, QImage::Format_ARGB32_Premultiplied);
	tile.fill(Qt::transparent);

	QPainter painter(&tile);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	QLinearGradient background(0, 0, 0, buttonImageSize);
	background.setColorAt(0.0, QColor("#0A0B0C"));
	background.setColorAt(1.0, QColor("#18191C"));
	painter.fillRect(tile.rect(), background);

	if(!member) {
		if(!message.isEmpty()) {
			QFont messageFont(QStringLiteral("Segoe UI"));
			messageFont.setPixelSize(14);
			messageFont.setWeight(QFont::DemiBold);
			painter.setFont(messageFont);
			painter.setPen(QColor("#DCDDDE"));
			painter.drawText(QRectF(12, 12, 120, 120),
							 Qt::AlignCenter | Qt::TextWordWrap, message);
		}
		return tile;
	}

	// 86 px becomes 43 physical pixels on 72 px Stream Deck keys. This is large
	// enough for faces to remain recognizable without crowding the labels.
	const QRectF avatarRect(29, 5, 86, 86);
	const bool isMuted = member->isMuted;
	const QColor ringColor = isMuted ? QColor("#ED4245")
									 : (isSpeaking ? QColor("#57F287") : QColor("#4F545C"));

	if(isMuted || isSpeaking) {
		QColor glow = ringColor;
		glow.setAlpha(28);
		painter.setBrush(Qt::NoBrush);
		painter.setPen(QPen(glow, 16, Qt::SolidLine, Qt::RoundCap));
		painter.drawEllipse(avatarRect);
		glow.setAlpha(58);
		painter.setPen(QPen(glow, 10, Qt::SolidLine, Qt::RoundCap));
		painter.drawEllipse(avatarRect);
		glow.setAlpha(105);
		painter.setPen(QPen(glow, 6, Qt::SolidLine, Qt::RoundCap));
		painter.drawEllipse(avatarRect);
	}

	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor("#24262B"));
	painter.drawEllipse(avatarRect);

	painter.save();
	QPainterPath avatarClip;
	avatarClip.addEllipse(avatarRect);
	painter.setClipPath(avatarClip);
	if(!avatar.isNull()) {
		const qreal sourceSide = qMin(avatar.width(), avatar.height());
		const QRectF sourceRect((avatar.width() - sourceSide) / 2.0,
								(avatar.height() - sourceSide) / 2.0,
								sourceSide, sourceSide);
		painter.drawImage(avatarRect, avatar, sourceRect);
	}
	else {
		drawAvatarPlaceholder(painter, avatarRect);
	}
	painter.restore();

	painter.setBrush(Qt::NoBrush);
	painter.setPen(QPen(ringColor, isMuted || isSpeaking ? 4.5 : 3.0,
						Qt::SolidLine, Qt::RoundCap));
	painter.drawEllipse(avatarRect);

	if(isMuted)
		drawMutedBadge(painter, avatarRect);

	QFont usernameFont(QStringLiteral("Segoe UI"));
	usernameFont.setPixelSize(20);
	usernameFont.setWeight(QFont::Bold);
	const QString username = formatButtonNick(member->nick);
	while(usernameFont.pixelSize() > 15
		  && QFontMetricsF(usernameFont).horizontalAdvance(username) > 132)
		usernameFont.setPixelSize(usernameFont.pixelSize() - 1);
	painter.setFont(usernameFont);
	painter.setPen(QColor("#F2F3F5"));
	painter.drawText(QRectF(5, 94, 134, 24), Qt::AlignCenter | Qt::TextSingleLine, username);

	QString status;
	QColor accentColor("#B9BBBE");
	if(isMuted) {
		status = QStringLiteral("MUTED");
		accentColor = QColor("#ED4245");
	}
	else if(isSpeaking) {
		status = QStringLiteral("TALKING");
		accentColor = QColor("#57F287");
	}

	// Keep the volume visible in every state. Previously SPEAKING/MUTED replaced
	// it, making the most important mixer value disappear precisely when active.
	const QRectF infoPill(6, 119, 132, 21);
	QColor pillColor = accentColor;
	pillColor.setAlpha(isMuted || isSpeaking ? 42 : 24);
	painter.setPen(QPen(QColor(255, 255, 255, 24), 1));
	painter.setBrush(pillColor);
	painter.drawRoundedRect(infoPill, 7, 7);

	const QString volume = QStringLiteral("%1%").arg(qRound(member->volume));
	if(status.isEmpty()) {
		QFont volumeFont(QStringLiteral("Segoe UI"));
		volumeFont.setPixelSize(18);
		volumeFont.setWeight(QFont::Bold);
		painter.setFont(volumeFont);
		painter.setPen(QColor("#F2F3F5"));
		painter.drawText(infoPill, Qt::AlignCenter | Qt::TextSingleLine, volume);
	}
	else {
		QFont statusFont(QStringLiteral("Segoe UI"));
		statusFont.setPixelSize(11);
		statusFont.setWeight(QFont::Bold);
		painter.setFont(statusFont);
		painter.setPen(accentColor);
		painter.drawText(QRectF(10, 119, 70, 21), Qt::AlignCenter | Qt::TextSingleLine, status);

		QFont volumeFont(QStringLiteral("Segoe UI"));
		volumeFont.setPixelSize(17);
		volumeFont.setWeight(QFont::Bold);
		painter.setFont(volumeFont);
		painter.setPen(QColor("#F2F3F5"));
		painter.drawText(QRectF(77, 119, 57, 21), Qt::AlignCenter | Qt::TextSingleLine, volume);
	}

	return tile;
}

} // namespace

void Action_VCMInfo::update_button() {
	const auto vcmp = voiceChannelMember();
	const VoiceChannelMember &vcm = vcmp ? *vcmp.mem : VoiceChannelMember::null;

	const bool isSpeaking = vcmp && plugin()->speakingVoiceChannelMembers.contains(vcm.userID);

	QString message;
	if(!plugin()->discord.isConnected())
		message = plugin()->discordStatusText();
	else if(!vcmp && plugin()->voiceChannelMembers.isEmpty()
			&& !plugin()->globalSetting("hideNobodyInVoiceChatText").toBool())
		message = QStringLiteral("NOBODY IN VOICE CHAT");

	const QImage avatar = vcmp
			? plugin()->getUserAvatar(vcm.userID, vcm.avatarID).toImage()
			: QImage{};
	const bool hasAvatar = !avatar.isNull();
	const QString renderKey = QStringLiteral("%1|%2|%3|%4|%5|%6|%7|%8|%9")
			.arg(message, vcm.userID, vcm.avatarID, vcm.nick,
				 QString::number(vcm.volume, 'f', 1),
				 QString::number(vcm.isMuted), QString::number(isSpeaking),
				 QString::number(hasAvatar), QString::number(bool(vcmp)));

	if(buttonRenderKey_ != renderKey) {
		buttonRenderKey_ = renderKey;
		if(title_ != QString()) {
			title_.clear();
			setTitle(QString());
		}

		const QImage tile = renderButtonTile(vcmp ? vcmp.mem : nullptr, avatar,
										 isSpeaking, message);
		const QString encodedTile = QStreamDeckPlugin::encodeImage(tile);
		setImage(encodedTile, 0);
		setImage(encodedTile, 1);
	}

	const int newState = isSpeaking ? 1 : 0;
	if(state_ != newState) {
		state_ = newState;
		setState(state_);
	}
}

void Action_VCMInfo::update_encoder() {
	const auto vcmp = voiceChannelMember();
	const VoiceChannelMember &vcm = vcmp ? *vcmp.mem : VoiceChannelMember::null;

	const bool isSpeaking = vcmp && plugin()->speakingVoiceChannelMembers.contains(vcm.userID);

	QJsonObject feedbackData;

	{
		QString newTitle;
		if(!plugin()->discord.isConnected())
			newTitle = plugin()->discordStatusText();
		else if(vcmp) {
			if(setting("showPaging").toBool())
				newTitle += QStringLiteral("%1/%2 ").arg(QString::number(vcmp.userIndex + 1), QString::number(plugin()->voiceChannelMembers.size()));

			newTitle += vcm.nick;
		}
		else if(plugin()->voiceChannelMembers.isEmpty() && !plugin()->globalSetting("hideNobodyInVoiceChatText").toBool())
			newTitle = QString("NOBODY IN VOICE");

		if(newTitle != title_) {
			title_ = newTitle;
			feedbackData.insert("title", newTitle);
		}
	}

	{
		const QString newLayout = vcmp ? "$B1" : "$X1";
		if(feedbackLayout_ != newLayout) {
			feedbackLayout_ = newLayout;
			setFeedbackLayout(newLayout);
		}
	}

	const QString newUserId = vcm.userID;
	if(userID_ != newUserId || !hasAvatar_ || isSpeaking) {
		userID_ = newUserId;

		const QImage avatar = plugin()->getUserAvatar(userID_, vcm.avatarID).toImage();
		hasAvatar_ = !avatar.isNull();

		QImage img(48, 48, QImage::Format_ARGB32);
		img.fill(Qt::transparent);
		{
			QPainter p(&img);
			p.setRenderHint(QPainter::Antialiasing);
			p.setRenderHint(QPainter::SmoothPixmapTransform);

			QPainterPath clipPath;
			clipPath.addEllipse(2, 2, 44, 44);
			p.setClipPath(clipPath);

			if(hasAvatar_) {
				p.drawImage(QRect(2, 2, 44, 44), avatar.scaled(44, 44, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
			}
			else if(vcmp) {
				static const QImage avatarPlaceholder = QImage("icons/icons8_user_72px.png").scaled(44, 44, Qt::KeepAspectRatio, Qt::SmoothTransformation);
				p.drawImage(QRect(2, 2, 44, 44), avatarPlaceholder);
			}

			p.setClipping(false);

			if(isSpeaking) {
				QPen pen(QColor(0x23, 0xa5, 0x5a), 3); // Discord Green
				p.setPen(pen);
				p.drawEllipse(2, 2, 44, 44);
			}
			else if(vcm.isMuted) {
				QPen pen(QColor(0xed, 0x42, 0x45), 2); // Discord Red
				p.setPen(pen);
				p.drawEllipse(2, 2, 44, 44);
			}
		}

		feedbackData.insert("icon", QStreamDeckPlugin::encodeImage(img));
	}

	const int newIndicatorValue = int(qRound(vcm.volume / 2));
	if(indicatorValue_ != newIndicatorValue) {
		indicatorValue_ = newIndicatorValue;
		feedbackData.insert("indicator", indicatorValue_);
	}

	{
		QString newValue;
		if(vcm.isMuted)
			newValue = "MUTED";
		else if(vcmp) {
			newValue = QStringLiteral("%1 %").arg(qRound(vcm.volume));
			if(isSpeaking)
				newValue = QStringLiteral("\U0001F3A4 ") + newValue;
		}

		if(feedbackValue_ != newValue) {
			feedbackValue_ = newValue;
			feedbackData.insert("value", newValue);
		}
	}

	setFeedback(feedbackData);
}

void Action_VCMInfo::onPressed() {
	executeAction(Action(setting("pressAction").toInt()));
}

void Action_VCMInfo::onTapped() {
	executeAction(Action(setting("tapAction").toInt()));
}

void Action_VCMInfo::onReleased() {
	// Force state update (because pressing switches it)
	setState(state_);
}

void Action_VCMInfo::onRotated(int delta) {
	const auto vcmp = voiceChannelMember();
	if(!vcmp)
		return;

	const int stepSize = plugin()->globalSetting("voiceChannelVolumeEncoderStep").toInt();
	plugin()->adjustVoiceChannelMemberVolume(*vcmp.mem, stepSize, delta);
}

void Action_VCMInfo::executeAction(Action_VCMInfo::Action a) {
	if(!plugin()->discord.isConnected())
		return;

	switch(a) {

		case Action::muteUnmute: {
			const auto vcmp = voiceChannelMember();
			if(!vcmp)
				return;

			auto vcm = vcmp.mem;

			vcm->isMuted ^= true;

			plugin()->discord.sendCommand(+QDiscord::CommandType::setUserVoiceSettings, QJsonObject{
				{"user_id", vcm->userID},
				{"mute",    vcm->isMuted},
			});
			emit plugin()->buttonsUpdateRequested();
			break;
		}

		case Action::nextUser:
			if(plugin()->voiceChannelMembers.isEmpty())
				return;
			device()->voiceChannelMemberIndexOffset = (device()->voiceChannelMemberIndexOffset + 1) % plugin()->voiceChannelMembers.size();
			emit plugin()->buttonsUpdateRequested();
			break;

		case Action::previousUser:
			if(plugin()->voiceChannelMembers.isEmpty())
				return;
			device()->voiceChannelMemberIndexOffset = (device()->voiceChannelMemberIndexOffset + plugin()->voiceChannelMembers.size() - 1) % plugin()->voiceChannelMembers.size();
			emit plugin()->buttonsUpdateRequested();
			break;

		case Action::none:
			break;

	}
}

void Action_VCMInfo::buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) {
	b.addCheckBox("hideNobodyInVoiceChatText", "Hide 'Nobody in voice chat' text (global)").linkWithGlobalSetting();

	static const QStringList actionSettings{
		"Mute/unmute user",
		"Next user",
		"Previous user",
		"None",
	};
	b.addComboBox("pressAction", "Press action", actionSettings).linkWithActionSetting();

	if(controller() == Controller::encoder) {
		b.addComboBox("tapAction", "Tap action", actionSettings).linkWithActionSetting();
		b.addCheckBox("showPaging", "Show paging").linkWithActionSetting();

		b.addSpinBox("voiceChannelVolumeEncoderStep", "Volume step").linkWithGlobalSetting();
		b.addMessage("Volume step is global for all volume encoders.");
	}


	VoiceChannelMemberAction::buildPropertyInspector(b);
}

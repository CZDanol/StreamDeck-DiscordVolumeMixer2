#include "action_vcminfo.h"

#include <QPainter>

#include <qtstreamdeck2/qstreamdeckpropertyinspectorbuilder.h>

#include "dvmplugin.h"

Action_VCMInfo::Action_VCMInfo() {
	connect(this, &QStreamDeckAction::keyDown, this, &Action_VCMInfo::onPressed);
	connect(this, &QStreamDeckAction::keyUp, this, &Action_VCMInfo::onReleased);

	connect(this, &QStreamDeckAction::dialPressed, this, &Action_VCMInfo::onPressed);
	connect(this, &QStreamDeckAction::dialRotated, this, &Action_VCMInfo::onRotated);
	connect(this, &QStreamDeckAction::touchTap, this, &Action_VCMInfo::onPressed);
}

void Action_VCMInfo::update() {
	if(controller() == Controller::keypad)
		update_button();
	else
		update_encoder();
}

void Action_VCMInfo::update_button() {
	const VoiceChannelMember *vcmp = voiceChannelMember();
	const VoiceChannelMember& vcm = vcmp ? *vcmp : VoiceChannelMember::null;

	const bool isSpeaking = vcmp && plugin()->speakingVoiceChannelMembers.contains(vcm.userID);

	const QString volumeStr = vcm.isMuted ? "MUTED" : QStringLiteral("%1 %").arg(QString::number(vcm.volume));

	{
		QString newTitle;
		if(!plugin()->discord.isConnected())
			newTitle = plugin()->discord.connectionError();
		else if(vcmp)
			newTitle = QStringLiteral("%1\n%3\n%2").arg(vcm.nick, volumeStr, isSpeaking ? ">>SPEAKING<<" : vcm.isMuted ? "##" : "");
		else if(plugin()->voiceChannelMembers.isEmpty() && !plugin()->globalSetting("hideNobodyInVoiceChatText").toBool())
			newTitle = QString("NOBODY\nIN\nVOICE CHAT");

		if(title_ != newTitle) {
			title_ = newTitle;
			setTitle(newTitle);
		}
	}

	const QString newUserId = vcm.userID;
	if(userID_ != newUserId || !hasAvatar_) {
		userID_ = newUserId;

		const QImage avatar = plugin()->discord.getUserAvatar(userID_, vcm.avatarID);
		hasAvatar_ = !avatar.isNull();

		QImage img(72, 72, QImage::Format_ARGB32);
		QPainter p(&img);
		img.fill(Qt::transparent);
		if(hasAvatar_) {
			p.setOpacity(0.5);
			p.drawImage(0, 0, avatar.scaled(72, 72, Qt::KeepAspectRatio, Qt::SmoothTransformation));
			p.setOpacity(1);
		}
		setImage(img, 0);

		static const QImage speakingDeco("icons/speaking_deco.png");
		p.drawImage(0, 0, speakingDeco);
		setImage(img, 1);
	}

	const int newState = isSpeaking ? 1 : 0;
	if(state_ != newState) {
		state_ = newState;
		setState(state_);
	}
}

void Action_VCMInfo::update_encoder() {
	const VoiceChannelMember *vcmp = voiceChannelMember();
	const VoiceChannelMember& vcm = vcmp ? *vcmp : VoiceChannelMember::null;

	const bool isSpeaking = vcmp && plugin()->speakingVoiceChannelMembers.contains(vcm.userID);

	QJsonObject feedbackData;

	{
		QString newTitle;
		if(!plugin()->discord.isConnected())
			newTitle = plugin()->discord.connectionError();
		else if(vcmp)
			newTitle = vcm.nick;
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
	if(userID_ != newUserId || !hasAvatar_) {
		userID_ = newUserId;

		const QImage avatar = plugin()->discord.getUserAvatar(userID_, vcm.avatarID);
		hasAvatar_ = !avatar.isNull();

		QImage img(48, 48, QImage::Format_ARGB32);
		QPainter p(&img);
		img.fill(Qt::transparent);
		if(hasAvatar_) {
			p.drawImage(0, 0, avatar.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		}
		else if(vcmp) {
			static const QImage avatarPlaceholder = QImage("icons/icons8_user_72px.png").scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
			p.drawImage(0, 0, avatarPlaceholder);
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
	VoiceChannelMember *vcm = voiceChannelMember();
	if(!vcm)
		return;

	vcm->isMuted ^= true;

	plugin()->discord.sendCommand(+QDiscord::CommandType::setUserVoiceSettings, QJsonObject{
		{"user_id", vcm->userID},
		{"mute",    vcm->isMuted},
	});
	emit plugin()->buttonsUpdateRequested();
}

void Action_VCMInfo::onReleased() {
	// Force state update (because pressing switches it)
	setState(state_);
}

void Action_VCMInfo::onRotated(int delta) {
	VoiceChannelMember *vcm = voiceChannelMember();
	if(!vcm)
		return;

	const int stepSize = plugin()->globalSetting("voiceChannelVolumeEncoderStep").toInt();
	plugin()->adjustVoiceChannelMemberVolume(*vcm, stepSize, delta);
}

void Action_VCMInfo::buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) {
	b.addCheckBox("hideNobodyInVoiceChatText", "Hide 'Nobody in voice chat' text (global)").linkWithGlobalSetting();

	if(controller() == Controller::encoder) {
		b.addSpinBox("voiceChannelVolumeEncoderStep", "Volume step").linkWithGlobalSetting();
		b.addMessage("Volume step is global for all volume encoders.");
	}

	VoiceChannelMemberAction::buildPropertyInspector(b);
}
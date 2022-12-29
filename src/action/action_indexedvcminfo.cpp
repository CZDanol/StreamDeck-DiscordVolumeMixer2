#include "action_indexedvcminfo.h"

#include <QPainter>

#include <qtstreamdeck2/qstreamdeckpropertyinspectorbuilder.h>

#include "dvmplugin.h"

Action_IndexedVCMInfo::Action_IndexedVCMInfo() {
	connect(this, &QStreamDeckAction::keyDown, this, &Action_IndexedVCMInfo::onClicked);
}

void Action_IndexedVCMInfo::update() {
	const VoiceChannelMember &vcm = voiceChannelMember();
	const bool isSpeaking = vcm.isValid && plugin()->speakingVoiceChannelMembers.contains(vcm.userID);

	const QString volumeStr = vcm.isMuted ? "MUTED" : QStringLiteral("%1 %").arg(QString::number(vcm.volume));

	QString newTitle;
	if(!plugin()->discord.isConnected())
		newTitle = plugin()->discord.connectionError();
	else if(vcm.isValid)
		newTitle = QStringLiteral("%1\n%3\n%2").arg(vcm.nick, volumeStr, isSpeaking ? ">>SPEAKING<<" : vcm.isMuted ? "##" : "");
	else if(plugin()->voiceChannelMembers.isEmpty() && !plugin()->globalSetting("hideNobodyInVoiceChatText").toBool())
		newTitle = QString("NOBODY\nIN\nVOICE CHAT");

	if(title_ != newTitle) {
		title_ = newTitle;
		setTitle(newTitle);
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

void Action_IndexedVCMInfo::onClicked() {
	VoiceChannelMember &vcm = voiceChannelMember();
	if(!vcm.isValid)
		return;

	vcm.isMuted ^= true;

	plugin()->discord.sendCommand(+QDiscord::CommandType::setUserVoiceSettings, QJsonObject{
		{"user_id", vcm.userID},
		{"mute", vcm.isMuted},
	});
	emit plugin()->buttonsUpdateRequested();
}

void Action_IndexedVCMInfo::buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) {
	b.addCheckBox("hideNobodyInVoiceChatText", "Hide NIVC", "Hide 'Nobody in voice chat' text (global)").linkWithGlobalSetting();

	IndexedVCMAction::buildPropertyInspector(b);
}
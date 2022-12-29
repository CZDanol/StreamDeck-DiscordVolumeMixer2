#include "action_indexeduser.h"

#include <QPainter>

#include <qtstreamdeck2/qstreamdeckpropertyinspectorbuilder.h>

#include "dvmplugin.h"

void Action_IndexedUser::update() {
	const VoiceChannelMember vcm = voiceChannelMember();
	const bool isEmpty = vcm.nick.isEmpty();
	const bool isSpeaking = !isEmpty && plugin()->speakingVoiceChannelMembers.contains(vcm.userID);

	const QString volumeStr = vcm.isMuted ? "MUTED" : QStringLiteral("%1 %").arg(QString::number(vcm.volume));

	QString newTitle;
	if(!plugin()->discord.isConnected())
		newTitle = plugin()->discord.connectionError();
	else if(!isEmpty)
		newTitle = QStringLiteral("%1\n%3\n%2").arg(vcm.nick, volumeStr, isSpeaking ? ">>SPEAKING<<" : vcm.isMuted ? "##" : "");
	else if(plugin()->voiceChannelMembers.isEmpty() && !setting("hide_nivc").toBool())
		newTitle = QString("NOBODY\nIN\nVOICE CHAT");

	if(title_ != newTitle) {
		title_ = newTitle;
		setTitle(newTitle);
	}

	const QString newUserId = vcm.userID;
	if(userID_ != newUserId || !hasAvatar_) {
		userID_ = newUserId;

		const QImage avatar = plugin()->discord.getUserAvatar(userID_, vcm.avatarID).scaled(72, 72, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		hasAvatar_ = !avatar.isNull();

		QImage img(72, 72, QImage::Format_ARGB32);
		QPainter p(&img);
		img.fill(Qt::transparent);
		p.setOpacity(0.5);
		p.drawImage(0, 0, avatar);
		p.setOpacity(1);
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

void Action_IndexedUser::buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) {
	b.addCheckBox("hide_nivc", "Hide 'Nobody in voice chat' text").linkWithActionSetting();

	IndexedUserAction::buildPropertyInspector(b);
}

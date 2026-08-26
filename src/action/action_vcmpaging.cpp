#include "action_vcmpaging.h"

#include <qtstreamdeck2/qstreamdeckpropertyinspectorbuilder.h>

#include "dvmplugin.h"

Action_VCMPaging::Action_VCMPaging() {
	connect(this, &QStreamDeckAction::initialized, this, &Action_VCMPaging::onInitialized);
	connect(this, &QStreamDeckAction::keyDown, this, &Action_VCMPaging::onPressed);
	connect(this, &QStreamDeckAction::keyUp, this, &Action_VCMPaging::onReleased);
}

Action_VCMPaging::~Action_VCMPaging() {
	if(isRegistered_)
		(isBackButton_ ? device()->vcmPrevPageButtonCount : device()->vcmNextPageButtonCount)--;
}

auto computeParams(Action_VCMPaging &b) {
	struct R {
		int pageCount = 0;
		int currentPage = 0;
		int maxOffset = 0;
	};

	const int step = qMax(1, b.setting("step").toInt());

	const int pageCount = static_cast<int>(b.plugin()->voiceChannelMembers.size() + step - 1) / step;
	return R{
		.pageCount = pageCount,
		.currentPage = b.device()->voiceChannelMemberIndexOffset / step,
		.maxOffset = (pageCount - 1) * step,
	};
}

void Action_VCMPaging::update() {
	const auto p = computeParams(*this);

	int newState;
	if(device()->vcmPrevPageButtonCount == 0 || device()->vcmNextPageButtonCount == 0)
		newState = (p.pageCount < 2);
	else
		newState = isBackButton_ ? (p.currentPage <= 0) : (p.currentPage >= p.pageCount - 1);

	if(state_ != newState) {
		state_ = newState;
		setState(newState);
	}

	const QString newTitle = newState ? "" : QStringLiteral("%1/%2").arg(p.currentPage + 1).arg(p.pageCount);
	if(title_ != newTitle) {
		title_ = newTitle;
		setTitle(newTitle);
	}
}

void Action_VCMPaging::buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) {
	b.addSpinBox("step", "Page step").linkWithActionSetting();
	DVMAction::buildPropertyInspector(b);
}


void Action_VCMPaging::onInitialized() {
	setSettingDefault("step", 1);

	// For backward compatibility reasons
	if(const auto v = setting("step"); v.isString())
		setSetting("step", v.toString().toInt());

	isBackButton_ = (actionUID() == "com.thomast.discordmixer.previouspage");
	(isBackButton_ ? device()->vcmPrevPageButtonCount : device()->vcmNextPageButtonCount)++;
	isRegistered_ = true;
}

void Action_VCMPaging::onPressed() {
	const auto p = computeParams(*this);
	if(p.pageCount < 2)
		return;

	auto &offset = device()->voiceChannelMemberIndexOffset;
	const int step = qMax(1, setting("step").toInt());
	int newOffset = offset + step * (isBackButton_ ? -1 : 1);
	if(newOffset > p.maxOffset)
		newOffset = 0;
	else if(newOffset < 0)
		newOffset = p.maxOffset;

	if(offset != newOffset) {
		state_ = -1;
		offset = newOffset;
		emit plugin()->buttonsUpdateRequested();
	}
}

void Action_VCMPaging::onReleased() {
	state_ = -1;
	update();
}

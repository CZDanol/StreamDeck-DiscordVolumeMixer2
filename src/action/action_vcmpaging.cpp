#include "action_vcmpaging.h"

#include <qtstreamdeck2/qstreamdeckpropertyinspectorbuilder.h>

#include "dvmplugin.h"

Action_VCMPaging::Action_VCMPaging() {
	connect(this, &QStreamDeckAction::initialized, this, &Action_VCMPaging::onInitialized);
	connect(this, &QStreamDeckAction::keyDown, this, &Action_VCMPaging::onPressed);
	connect(this, &QStreamDeckAction::keyUp, this, &Action_VCMPaging::onReleased);
}

Action_VCMPaging::~Action_VCMPaging() {
	(isBackButton_ ? device()->vcmPrevPageButtonCount : device()->vcmNextPageButtonCount)--;
}

auto computeParams(Action_VCMPaging &b) {
	struct R {
		int pageCount = 0;
		int currentPage = 0;
		int maxOffset = 0;
	};

	qDebug() << b.settings();
	const int step = b.setting("step").toInt();
	if(!step)
		return R{};

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

	isBackButton_ = (actionUID() == "cz.danol.discordmixer.previousPage");
	(isBackButton_ ? device()->vcmPrevPageButtonCount : device()->vcmNextPageButtonCount)++;
}

void Action_VCMPaging::onPressed() {
	const auto p = computeParams(*this);

	auto &offset = device()->voiceChannelMemberIndexOffset;
	int newOffset = offset + setting("step").toInt() * (isBackButton_ ? -1 : 1);
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

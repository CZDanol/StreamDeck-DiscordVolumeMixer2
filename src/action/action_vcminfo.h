#pragma once

#include "voicechannelmemberaction.h"

class Action_VCMInfo : public VoiceChannelMemberAction {
Q_OBJECT

public:
	enum class Action {
		muteUnmute,
		nextUser,
		previousUser,
		none
	};

public:
	Action_VCMInfo();

public slots:
	virtual void update() override;

protected:
	virtual void buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) override;

private:
	void update_button();
	void update_encoder();

private slots:
	void onPressed();
	void onReleased();
	void onRotated(int delta);
	void onTapped();

private:
	void executeAction(Action a);

private:
	QString title_ = "###";
	QString userID_;
	bool hasAvatar_ = false;
	int state_ = -1;

private:
	QString feedbackLayout_ = "###";
	int indicatorValue_ = -1;
	QString feedbackValue_ = "###";

};

#pragma once

#include "indexedvcmaction.h"

class Action_IndexedVCMInfo : public IndexedVCMAction {
Q_OBJECT

public:
	Action_IndexedVCMInfo();

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

private:
	QString title_;
	QString userID_;
	bool hasAvatar_ = false;
	int state_ = -1;

private:
	QString feedbackLayout_;
	int indicatorValue_ = -1;
	QString feedbackValue_;

};

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

private slots:
	void onClicked();

private:
	QString title_;
	QString userID_;
	bool hasAvatar_ = false;
	int state_ = -1;

};

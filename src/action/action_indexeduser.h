#pragma once

#include "indexeduseraction.h"

class Action_IndexedUser : public IndexedUserAction {
Q_OBJECT

public:
	Action_IndexedUser();

public slots:
	virtual void update() override;

protected:
	virtual void buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) override;

private:
	QString title_;
	QString userID_;
	bool hasAvatar_ = false;
	int state_ = -1;

};

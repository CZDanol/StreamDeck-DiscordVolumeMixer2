#pragma once

#include "dvmaction.h"

class Action_VCMPaging : public DVMAction {

public:
	Action_VCMPaging();
	~Action_VCMPaging();

public slots:
	void update();

protected:
	virtual void buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) override;

private:
	void onInitialized();
	void onPressed();
	void onReleased();

private:
	QString title_ = "###";
	int state_ = -1;
	bool isBackButton_ = false;

};

#pragma once

#include <QTimer>

#include "indexedvcmaction.h"

class Action_IndexedVCMVolume : public IndexedVCMAction {

public:
	Action_IndexedVCMVolume();

public:
	virtual void update() override;

protected:
	virtual void buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) override;

private slots:
	void onInitialized();
	void onPressed();
	void onReleased();
	void trigger();

private:
	bool isVolumeDown_ = false;
	int state_ = -1;

private:
	int repeatSkip_ = 0;
	QTimer repeatTimer_;

};

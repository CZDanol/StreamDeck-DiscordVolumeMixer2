#pragma once

#include <qtstreamdeck2/qstreamdeckaction.h>

#include "dvmdevice.h"

class DVMAction : public QStreamDeckActionT<DVMDevice> {
Q_OBJECT

public:
	DVMAction();

public slots:
	virtual void update() {};

protected:
	virtual void buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) override;

private slots:
	void onInitialized();

};
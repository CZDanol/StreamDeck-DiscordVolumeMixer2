#pragma once

#include <qtstreamdeck2/qstreamdeckaction.h>

#include "dvmdevice.h"

class DVMAction : public QStreamDeckActionT<DVMDevice> {
Q_OBJECT

protected:
	virtual void buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) override;

};
#pragma once

#include <qtstreamdeck2/qstreamdeckdevice.h>

#include "declares.h"

class DVMDevice : public QStreamDeckDeviceT<DVMPlugin> {
Q_OBJECT
};

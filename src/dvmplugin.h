#pragma once

#include <qtstreamdeck2/qstreamdeckplugin.h>

#include "declares.h"

class DVMPlugin : public QStreamDeckPluginT<DVMPluginDevice> {
Q_OBJECT
};

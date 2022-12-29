#pragma once

#include <qtstreamdeck2/qstreamdeckdevice.h>

#include "declares.h"

class DVMDevice : public QStreamDeckDeviceT<DVMPlugin> {
Q_OBJECT

public:
	/// Used for paging voice channel members
	int voiceChannelMemberIndexOffset = 0;

	int vcmNextPageButtonCount = 0, vcmPrevPageButtonCount = 0;

};

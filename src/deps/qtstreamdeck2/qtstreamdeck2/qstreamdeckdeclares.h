#pragma once

class QStreamDeckPlugin;
class QStreamDeckDevice;
class QStreamDeckAction;

class QStreamDeckPropertyInspectorBuilder;

struct QStreamDeckEvent;

using QStreamDeckActionUID = QString;
using QStreamDeckActionContext = QString;
using QStreamDeckDeviceContext = QString;

using QStreamDeckPropertyInspectorCallback = std::function<void(const QStreamDeckEvent &)>;
#include "dvmaction.h"

#include <qtstreamdeck2/qstreamdeckpropertyinspectorbuilder.h>

void DVMAction::buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) {
	b.addSection("Global settings");
	b.addMessage("For configuration instructions, see the <a href=\"openUrl(\\\"https://github.com/CZDanol/StreamDeck-DiscordVolumeMixer2\\\")\">GitHub page</a>.");
	b.addLineEdit("clientID", "Client ID").linkWithGlobalSetting();
	b.addLineEdit("clientSecret", "Client secret").linkWithGlobalSetting();
}

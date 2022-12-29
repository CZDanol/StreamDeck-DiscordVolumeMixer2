#include "dvmaction.h"

#include <qtstreamdeck2/qstreamdeckpropertyinspectorbuilder.h>

void DVMAction::buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) {
	b.addSection("Global settings");
	b.addMessage("For configuration instructions, see the <a href=\"javascript: openUrl('https://github.com/CZDanol/StreamDeck-DiscordVolumeMixer2');\">GitHub page</a>.");
	b.addLineEdit("client_id", "Client ID").linkWithGlobalSetting();
	b.addLineEdit("client_secret", "Client secret").linkWithGlobalSetting();
}

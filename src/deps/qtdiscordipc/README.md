# QtDiscordIPC
Discord RPC/IPC API for Qt implemented using QLocalSocket.
This API allows to control the locally running Discord application. It's not particularly well implemented, I've made it for my [Discord Volume Mixer for Stream Deck project](https://github.com/CZDanol/streamdeck-discordmixer).

API documentation: https://discord.com/developers/docs/topics/rpc

**Cudos to [this guy on Stack Overflow](https://stackoverflow.com/a/68958800/5290264).** His answer was the only information source I was able to find about the Discord IPC protocol. Without that I wouldn't be able to make this lib.

Uses OAuth authentication. After authentized, stores the auth data in discordOauth.json so that the app doesn't have to authenticate each time.

Asynchronous usage, using Qt event system (similar to QNetworkReply).

> **See the [Discord Volume Mixer 2](https://github.com/CZDanol/StreamDeck-DiscordVolumeMixer2) github repo for example usage and setup instructions.**

## Requirements
Requires Qt Core and Network.
Tested on MSVC 2019 x64, Qt 6.2.1, C++17.
# QtStreamDeck
Qt library for creating Elgato Stream Deck plugins.

* Provides wrapper and some boilerplate over the Websocket-based API.
* Provides abstractions to make your plugin OOP-based.
* Also provides elegant API for Property inspector - you don't have to create a webpage for each action, you can generate the fields from the code.

## Installation and usage
> **See the [Discord Volume Mixer 2](https://github.com/CZDanol/StreamDeck-DiscordVolumeMixer2) github repo for example usage and setup instructions.**

* Add the `qtstreamdeck2` folder to your project (contains headers and sources).
* Make sure the contents of the `dist` folder gets distributed with your plugin.
* In `manifest.json`, set `PropertyInspectorPath` to `qtstreamdeck2/propertyinspector.html`
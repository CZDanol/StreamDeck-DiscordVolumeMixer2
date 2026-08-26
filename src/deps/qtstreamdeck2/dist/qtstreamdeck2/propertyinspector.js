var websocket = null,
    uuid = null,
    actionInfo = {};

const commandFunctions = {
    updateBody: function (msg) {
        document.getElementById("body").innerHTML = msg.content;
    },
};

function connectElgatoStreamDeckSocket(inPort, inUUID, inRegisterEvent, inInfo, inActionInfo) {
    uuid = inUUID;
    // please note: the incoming arguments are of type STRING, so
    // in case of the inActionInfo, we must parse it into JSON first
    actionInfo = JSON.parse(inActionInfo); // cache the info
    websocket = new WebSocket('ws://localhost:' + inPort);

    // if connection was established, the websocket sends
    // an 'onopen' event, where we need to register our PI
    websocket.onopen = function () {
        // register property inspector to Stream Deck
        websocket.send(JSON.stringify({
            event: inRegisterEvent,
            uuid: inUUID
        }));
    }

    websocket.onmessage = function (ev) {
        let evd = JSON.parse(ev.data);
        if (evd.event !== "sendToPropertyInspector")
            return;

        let msg = evd.payload;
        commandFunctions[msg.cmd](msg);
    };
}

function notifyValueChanged(elementID, value) {
    websocket.send(JSON.stringify({
        event: "sendToPlugin",
        context: uuid,
        action: actionInfo.action,
        payload: {
            cmd: "valueChanged",
            element: elementID,
            value: value
        },
    }));
}

function openUrl(url) {
    websocket.send(JSON.stringify({
        event: "openUrl",
        payload: {
            url: url
        },
    }));
}
import { app } from "electron"
import { initState } from "@app/utils/state"
import * as events from "@app/main/events"
import * as effects from "@app/main/effects"

function init(): void {
    const state = {
        appWindowRequested: true,
    }

    initState(
        state,
        [
            events.listenState,
            events.listenAccess,
            events.listenAppActivate,
            events.listenAppQuit,
        ],
        [
            effects.launchAuthWindow,
            effects.launchProtocol,
            effects.storeAccess,
            effects.broadcastState,
            effects.closeAllWindows,
            // effects.logState,
        ]
    )
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.whenReady().then(init)

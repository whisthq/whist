import { app } from "electron"
import { initState } from "@app/utils/state"
import * as events from "@app/main/events"
import * as effects from "@app/main/effects"

function init(): void {
    const state = {
        email: "",
        accessToken: "",
        refreshToken: "",
        protocolLoading: false,
        appWindowRequested: true,
    }
    // Use events.clearPersistOnStart for development/testing
    // if you want the auth window to pop up. Otherwise, your credentials will
    // be persisted and you'll launch the protocol right away.
    //
    //
    // Use effects.logState to print out every state change to the main thread
    // console.
    initState(
        state,
        [
            // events.clearPersistOnStart,
            events.loadPersistOnStart,
            events.verifyTokenOnStart,
            events.listenState,
            events.listenAppActivate,
            events.listenAppQuit,
        ],
        [
            effects.launchAuthWindow,
            effects.launchProtocol,
            effects.persistState,
            effects.broadcastState,
            effects.closeAllWindows,
            effects.logState,
        ]
    )
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.whenReady().then(init)

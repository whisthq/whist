import { app } from "electron"
import { loginFailure } from "@app/main/login"
import { protocolLaunchFailure } from "@app/main/protocol"
import {
    containerCreateFailure,
    containerAssignFailure,
} from "@app/main/container"

import { ipcState, appReady } from "@app/main/events"
import { concat, race, combineLatest, of } from "rxjs"
import { pluck, withLatestFrom, skip, map } from "rxjs/operators"
import {
    closeWindows,
    createAuthErrorWindow,
    createContainerErrorWindow,
    createProtocolErrorWindow,
} from "@app/utils/windows"

const errorRelaunchRequest = ipcState
    .pipe(pluck("errorRelaunchRequest"))
    .subscribe((req) => {
        if (req) {
            app.relaunch()
            app.exit()
        }
    })

concat(
    appReady,
    race(
        combineLatest(loginFailure, of(createAuthErrorWindow)),
        combineLatest(containerAssignFailure, of(createContainerErrorWindow)),
        combineLatest(containerAssignFailure, of(createContainerErrorWindow)),
        combineLatest(protocolLaunchFailure, of(createProtocolErrorWindow))
    )
)
    .pipe(skip(1)) // skip the appReady emit
    .subscribe(([_failure, createWindow]) => {
        closeWindows(), createWindow()
    })

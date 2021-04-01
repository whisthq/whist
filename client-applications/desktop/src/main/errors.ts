import { app } from "electron"
import { loginFailure } from "@app/main/login"
import { protocolLaunchFailure } from "@app/main/protocol"
import {
    containerCreateFailure,
    containerAssignFailure,
} from "@app/main/container"

import { ipcState, appReady } from "@app/main/events"
import { combineLatest } from "rxjs"
import { pluck } from "rxjs/operators"
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

combineLatest(appReady, loginFailure).subscribe(() => {
    closeWindows()
    createAuthErrorWindow((win: any) => win.show())
})

combineLatest(appReady, containerCreateFailure).subscribe(() => {
    closeWindows()
    createContainerErrorWindow((win: any) => win.show())
})

combineLatest(appReady, containerAssignFailure).subscribe(() => {
    closeWindows()
    createContainerErrorWindow((win: any) => win.show())
})

combineLatest(appReady, protocolLaunchFailure).subscribe(() => {
    closeWindows()
    createProtocolErrorWindow((win: any) => win.show())
})

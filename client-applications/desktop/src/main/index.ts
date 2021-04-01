import "@app/main/container"
import "@app/main/debug"
import "@app/main/errors"
import "@app/main/protocol"
import "@app/main/user"

import { ipcState } from "@app/main/events"
import { loginLoading, loginWarning } from "@app/main/login"

import { ipcBroadcast } from "@app/utils/state"
import { getWindows } from "@app/utils/windows"
import { combineLatest } from "rxjs"
import { startWith, mapTo } from "rxjs/operators"
import { WarningLoginInvalid } from "@app/utils/constants"

// Broadcast state to all renderer windows.
combineLatest(
    ipcState,
    loginLoading.pipe(startWith(false)),
    loginWarning.pipe(mapTo(WarningLoginInvalid), startWith(null))
).subscribe(([state, loginLoading, loginWarning]) =>
    ipcBroadcast({ ...state, loginLoading, loginWarning }, getWindows())
)

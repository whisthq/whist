import { eventIPC } from "@app/main/events/ipc"

import { pluck } from "rxjs/operators"

import { eventAppReady, eventAppQuit } from "@app/main/events/app"
import { logDebug, logError } from "@app/utils/logging"
import { combineLatest } from "rxjs"

import {
    protocolLaunchProcess,
    protocolLaunchSuccess,
    protocolLaunchFailure,
} from "@app/main/observables/protocol"

import { loginRequest } from "@app/main/observables/login"

combineLatest(
    eventIPC.pipe(pluck("email")),
    eventAppReady
).subscribe(([email, _appReady]) => logDebug(email as string, "App ready event emitted"))

combineLatest(
    eventIPC.pipe(pluck("email")),
    protocolLaunchSuccess
).subscribe(([email, _success]) => logDebug(email as string, "Protocol successfully launched"))

combineLatest(
    eventIPC.pipe(pluck("email")),
    protocolLaunchFailure
).subscribe(([email, _success]) =>
    logError(email as string, "Protocol launch failed")
)

combineLatest(
    eventIPC.pipe(pluck("email")),
    protocolLaunchProcess
).subscribe(([email, _success]) => logError(email as string, "spawn() protocol command sent"))

loginRequest.pipe(pluck("email")).subscribe(email => logDebug(email as string, "Login attempt"))
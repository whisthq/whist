import { eventIPC } from "@app/main/events/ipc"

import { pluck } from "rxjs/operators"

import { eventAppReady } from "@app/main/events/app"
import { logDebug, logError } from "@app/utils/logging"
import { Observable, combineLatest } from "rxjs"

import {
    protocolLaunchProcess,
    protocolLaunchSuccess,
    protocolLaunchFailure,
} from "@app/main/observables/protocol"

import { loginRequest } from "@app/main/observables/login"

const logWithEmail = (observable: Observable<any>, logs: string) => combineLatest(
    eventIPC.pipe(pluck("email")),
    observable 
).subscribe(([email, _observable]) => logDebug(email as string, logs))

const pluckLog = (observable: Observable<any>, logs: string) => observable.pipe(
    pluck("email")
).subscribe(email => logDebug(email as string, logs))

// For logging observables that don't emit an email
logWithEmail(eventAppReady, "App ready event emitted")
logWithEmail(protocolLaunchSuccess, "Protocol successfully launched")
logWithEmail(protocolLaunchFailure, "Protocol launch failed")
logWithEmail(protocolLaunchProcess, "spawn() protocol command sent")

// For logging observables that already emit an email
pluckLog(loginRequest, "Login attempt")

import { eventIPC } from "@app/main/events/ipc"

import { pluck } from "rxjs/operators"

import { logDebug, logError } from "@app/utils/logging"
import { Observable, combineLatest } from "rxjs"

import * as e from "@app/main/events/app"
import * as h from "@app/main/observables/host"
import * as c from "@app/main/observables/container"
import * as p from "@app/main/observables/protocol"
import * as s from "@app/main/observables/signup"
import * as l from "@app/main/observables/login"

const logWithEmail = (observable: Observable<any>, logs: string) =>
    combineLatest(
        eventIPC.pipe(pluck("email")),
        observable
    ).subscribe(([email, _observable]) => logDebug(email as string, logs))

const pluckLog = (observable: Observable<any>, logs: string) =>
    observable
        .pipe(pluck("email"))
        .subscribe((email) => logDebug(email as string, logs))

// For logging observables that don't emit an email
logWithEmail(e.eventAppReady, "App ready event emitted")
logWithEmail(p.protocolLaunchSuccess, "Protocol successfully launched")
logWithEmail(p.protocolLaunchFailure, "Protocol launch failed")
logWithEmail(p.protocolLaunchProcess, "spawn() protocol command sent")

// For logging observables that already emit an email
pluckLog(l.loginRequest, "Login attempt")

l.loginFailure.subscribe((err) =>
    logError("loginFailure", "Could not login", err)
)

s.signupFailure.subscribe((err) =>
    logError("signupFailure", "Could not signup", err)
)

c.containerCreateFailure.subscribe((err) =>
    logError("containerCreateFailure", "Could create container", err)
)

c.containerAssignFailure.subscribe((err) =>
    logError("containerAssignFailure", "Could assign container", err)
)

h.hostInfoFailure.subscribe((err) =>
    logError("hostInfoFailure", "Could not get host service info", err)
)

h.hostConfigFailure.subscribe((err) =>
    logError("hostConfigFailure", "Could not set host service config", err)
)

p.protocolLaunchFailure.subscribe((err) =>
    logError("protocolLaunchFailure", "Could not launch protocol", err)
)

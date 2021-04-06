// This file is a utility for debugging observables throughout the application.
// By subscribing to observables here, we can see their values in our console
// every time they emit.
//
// First, the module containing your new observables must be imported at the
// top of this file. For example:
//
// import * as mymodule from @app/main/mymodule
//
// This will store a dictionary of the variables in @app/main/mymodule under
// the symbol mymodule. You must then add mymodule to the "modules" list,
// right below the import statements in this file.
//
// Finally, add the name of the observable you want to debug to the schema
// dictionary. The key should be the name of your observable, and the value
// is a tuple of ["debug message", trasnformFunction (optional)]. The
// transformFunction will receive the emitted value from the observable, and
// whatever the transformFunction returns will be printed in the debug
// message. This is useful to only print the fields you care about in a large
// object.
//
// If you don't pass a transformFunction, the entire value will be printed.
// If you pass null as the transformFunction, only the debug message will
// be printed, not the value.
// If the observable name set as the schema key doesn't exist in any of the
// modules, it will be ignored.

import { isObservable } from "rxjs"
import { identity, pick } from "lodash"
import { logDebug } from "@app/utils/logging"
import * as login from "@app/main/observables/login"
import * as host from "@app/main/observables/host"
import * as signup from "@app/main/observables/signup"
import * as container from "@app/main/observables/container"
import * as protocol from "@app/main/observables/protocol"
import * as user from "@app/main/observables/user"
import * as error from "@app/main/observables/error"
import * as events from "@app/main/events"

const modules = [login, container, protocol, user, error, signup, events, host]

type DebugSchema = {
    [title: string]:
        | [string]
        | [string, null]
        | [string, (...args: any[]) => any]
}

const schema: DebugSchema = {
    eventIPC: ["value:"],
    userEmail: ["value:"],
    userAccessToken: ["value:"],
    userRefreshToken: ["value:"],
    userConfigToken: ["value:"],
    signupRequest: ["value:"],
    signupLoading: ["value:"],
    signupWarning: ["user already exists", null],
    signupFailure: ["value:"],
    signupSuccess: [
        "printing only tokens:",
        ({ json }) => ({
            accessToken: json.access_token?.substring(0, 6) + "...",
            refreshToken: json.refresh_token?.substring(0, 6) + "...",
        }),
    ],
    loginRequest: ["value:"],
    loginLoading: ["value:"],
    loginWarning: ["logged in with invalid credentials", null],
    loginFailure: ["value:"],
    loginSuccess: [
        "value:",
        ({ json }) => JSON.stringify(json)
    ],
    containerCreateRequest: ["value:"],
    containerCreateLoading: ["value:"],
    containerCreateSuccess: ["printing only taskID:", ({ json }) => json.ID],
    containerCreateFailure: ["error:"],
    containerAssignRequest: ["value:"],
    containerAssignLoading: ["value:"],
    containerAssignPolling: ["polling containerAssign", null],
    containerAssignSuccess: ["value:"],
    containerAssignFailure: ["error:"],
    protocolLaunchProcess: [
        "value:",
        ({ connected, exitCode, pid }) => ({
            connected,
            exitCode,
            pid,
        }),
    ],
    protocolLaunchLoading: ["value:"],
    protocolLaunchSuccess: ["value:"],
    protocolLaunchFailure: ["error:"],
    protocolCloseRequest: [
        "printing subset of protocol object:",
        ([protocol]) =>
            pick(protocol, [
                "killed",
                "connected",
                "exitCode",
                "signalCode",
                "pid",
            ]),
    ],
    hostInfoRequest: ["value:"],
    hostInfoSuccess: ["value:"],
    hostInfoFailure: ["value:"],
    hostConfigRequest: ["value:"],
    hostConfigProcess: [
        "printing only status, json:",
        (obj) => pick(obj, ["status", "json"]),
    ],
    hostConfigSuccess: [
        "printing only status, json:",
        (obj) => pick(obj, ["status", "json"]),
    ],
    hostConfigFailure: ["error:"],
    errorRelaunchRequest: ["relaunching app due to error!"],
}

const symbols = modules.reduce((acc, m) => ({ ...acc, ...m }), {}) as any

for (let key in symbols)
    if (isObservable(symbols[key as string]) && schema[key]) {
        symbols[key].subscribe((...args: any) => {
            let [message, func] = schema[key]
            if (func === undefined) func = identity
            let data = func ? func(...args) : undefined
            logDebug(key, message, data)
        })
    }

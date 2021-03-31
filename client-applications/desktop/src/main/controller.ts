import { Event, State, StateChannel, ipcBroadcast } from "@app/utils/state"
import { ipcMain } from "electron"
import { app, BrowserWindow } from "electron"
import { store, persistClear } from "@app/utils/persist"
import { fractalLoginWarning } from "@app/utils/constants"
import EventEmitter from "node:events"
import {
    containerCreate,
    containerInfo,
    responseContainerState,
    responseContainerLoading,
    responseContainerID,
} from "@app/utils/protocol"
import { createAuthWindow, createErrorWindow, getWindows } from "@app/utils/windows"
import {
    emailLogin,
    responseAccessToken,
    responseRefreshToken,
    responseConfigToken,
} from "@app/utils/api"
import _ from "lodash"
import { persist, persistLoad } from "@app/utils/persist"
import { fromEvent, combineLatest, merge, of, interval, zip } from "rxjs"
import {
    map,
    tap,
    share,
    pluck,
    filter,
    distinct,
    switchMap,
    exhaustMap,
    withLatestFrom,
    mapTo,
    take,
    takeWhile,
} from "rxjs/operators"

// Set up Electron application listeners.
// We use the Electron event emitters on the app object to listen for
// important application lifecycle events.

fromEvent(app as EventEmitter, "window-all-closed").subscribe(() => {
    // On macOS it is common for applications and their menu bar
    // to stay active until the user quits explicitly with Cmd + Q
    if (process.platform !== "darwin") app.quit()
})

export const appReady = fromEvent(app as EventEmitter, "ready")

export const appActivations = fromEvent(app as EventEmitter, "activate")

// Here, we set up our listener on the IPC channel to pass state between
// main and render thread. We share() the resulting Observable, because
// it will have multiple observers, and share() saves us from calling
// fromEvent over and over.
export const ipcStates = fromEvent(ipcMain, StateChannel).pipe(
    map((args) => args as [any, Partial<State>]),
    map(([_event, state]) => state),
    share()
)

// Load the persisted state from the Electron store cache.
export const persisted = appReady.pipe(map(() => persistLoad()))

// Upon clicking the login button we receive a new login request, which is a
// tuple of email, password.
export const userLoginRequests = ipcStates
    .pipe(pluck("loginRequest"))
    .pipe(filter((req) => (req?.email && req?.password ? true : false)))
    .pipe(share())

// We can either get the user's email from the persisted state, or from
// the IPC channel to the auth window. merge will take whatever comes first.
// pluck extracts a value from a given key from an object.
// Here, loading from persistence is commented out so that we see the auth
// window each time during development.
export const userEmail = merge(
    // persisted.pipe(pluck("email")),
    userLoginRequests.pipe(map((req) => req!.email))
)

// The user's password is pulled from IPC state, and not persisted.
export const userPassword = userLoginRequests.pipe(map((req) => req!.password))

// export const userConfirmPassword = ipcStates.pipe(pluck("confirmPassword"))

// Here we start to really think "reactively". We ask ourselves: "What is a
// userLoginResponse"? The answer is that it's a combination of information:
// userEmail and userPassword, and every time get a new userLoginRequest,
// we want to use our latest userEmail and userPassword to get a new
// userLoginRequest. The details of timing, firing events etc. are
// handled by rxjs.
export const userLoginResponses = userLoginRequests
    .pipe(withLatestFrom(userEmail, userPassword))
    .pipe(exhaustMap(([_req, email, pw]) => emailLogin(email!, pw!)))
    .pipe(share())

// Similarly, we can ask: what is userLoginLoading? It's just a
// state that's true when we have a userLoginRequest, and turns false
// when we have a userLoginResponse. So we simply map the events from
// each of those respective channels to the correct loading value.
export const userLoginLoading = merge(
    userLoginRequests.pipe(mapTo(true)),
    userLoginResponses.pipe(mapTo(false))
)

export const userLoginWarnings = zip(
    userLoginRequests,
    userLoginResponses
).pipe(
    map(([_, r]) => (responseAccessToken(r) ? undefined : "Login warning"))
)

// We use helper functions from api.ts to extract the value of the tokens
// from the userLoginResponse, with is a core-ts post response.
export const userAccessToken = userLoginResponses.pipe(map(responseAccessToken))

export const userRefreshToken = userLoginResponses.pipe(
    map(responseRefreshToken)
)

// Getting the config token looks a little different from the other tokens,
// because it's an async process due to the cryptography.
// We use switchMap to automatically resolve the promise.
export const userConfigToken = userLoginResponses
    .pipe(withLatestFrom(userPassword))
    .pipe(switchMap(([res, pw]) => responseConfigToken(pw!, res)))

// We take a similar strategy to the userLoginRequest with our
// containerCreateRequest and protocolLaunchRequest. We'll have
// seperate observables for each of request, loading, and response,
// and extractor functions (like responseContainerID) to pull data out
// of the responses.
export const containerCreateRequests = combineLatest([
    appReady,
    userAccessToken,
])
    .pipe(map(([_, token]) => token))
    .pipe(withLatestFrom(userEmail))

export const containerCreateResponses = containerCreateRequests.pipe(
    exhaustMap(([token, email]) => containerCreate(email!, token!))
)

export const containerCreateLoading = merge(
    containerCreateRequests.pipe(mapTo(true)),
    containerCreateResponses.pipe(mapTo(false))
)

export const containerCreateSuccess = containerCreateRequests

export const containerTaskID = containerCreateResponses.pipe(
    map(responseContainerID)
)

export const containerInfoResponses = containerCreateResponses
    .pipe(withLatestFrom(containerTaskID, userAccessToken))
    .pipe(
        exhaustMap(([_res, id, token]) =>
            interval(1000)
                .pipe(switchMap(() => containerInfo(id!, token!)))
                .pipe(takeWhile((res) => !responseContainerLoading(res), true))
        )
    )

export const containerInfoState = containerInfoResponses.pipe(
    map(responseContainerState)
)

export const containerInfoReady = combineLatest([
    containerInfoState.pipe(filter((status) => status === "SUCCESS")),
])

export const protocolLaunchRequests = combineLatest([appReady, userAccessToken])

export const protocolLaunchResponses = protocolLaunchRequests
    .pipe(withLatestFrom())
    .pipe(withLatestFrom())

export const protocolLaunchLoading = merge(
    protocolLaunchRequests.pipe(mapTo(true)),
    protocolLaunchResponses.pipe(mapTo(false))
)

// Set up IPC state broadcast
// This is the mirror to the "fromEvent" observable above, which receives the
// state from the IPC channel. Here, we send back the state that the renderer
// thread cares about. We send back everything we received, plus the login
// loading state.
combineLatest([
    ipcStates,
    userLoginLoading,
    userLoginWarnings,
]).subscribe(([state, loginLoading, loginWarning]) =>
    ipcBroadcast({ ...state, loginLoading, loginWarning }, getWindows())
)

// We listen to both "appReady" and "appActivation" events to show the auth
// window.
export const windowAuth = merge(appReady, appActivations).subscribe(() => {
    if (getWindows().length === 0) createAuthWindow((win: any) => win.show())
})

// We persist our latest email and accessTokens to state as we receive them.
combineLatest({ userEmail, userAccessToken }).subscribe(persist)

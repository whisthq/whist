// This is the application's main entry point. No code should live here, this
// file's only purpose is to "initialize" other files in the application by
// importing them.
//
// This file should import every other file in the top-level of the "main
// folder. While many of those files import each other and would run anyways,
// we import everything here first for the sake of being explicit.
//
// If you've created a new file in "main/" and you're not seeing it run, you
// probably haven't imported it here.

// import "@app/main/observables"
// import "@app/main/events"
// import "@app/main/effects"

import { app } from "electron"
import { gates, Flow } from "@app/utils/gates"
import { BehaviorSubject, merge, fromEvent, zip, combineLatest } from "rxjs"
import { switchMap, pluck, sample, map } from "rxjs/operators"
import { signupFlow } from "@app/main/observables/signup"
import { loginFlow } from "@app/main/observables/login"
import { protocolLaunchFlow } from "@app/main/observables/protocol"
import {
  containerCreateFlow,
  containerPollingFlow,
} from "@app/main/observables/container"
import { hostServiceFlow } from "@app/main/observables/host"
import { loginAction, signupAction } from "@app/main/events/actions"
import { store, persist, onPersistChange } from "@app/utils/persist"
import { protocolStreamInfo } from "@app/utils/protocol"
import { has, every, omit } from "lodash"
import { createAuthWindow } from "@app/utils/windows"

// Auth is currently the only flow that requires any kind of persistence,
// so for now we might as well just leave the persistence setup close to
// the auth flow.
//
// We use a BehaviorSubject because we need an "initial value" for the
// persistStore as soon as the application loads.
//

const persistStore = new BehaviorSubject(store.store)
onPersistChange((store: any) => persistStore.next(store))

store.onDidAnyChange((newStore: any, _oldStore: any) => {
  persistStore.next(newStore)
})

const persistedCredsValid = (store: any) =>
  every(["email", "accessToken", "configToken", "refreshToken"], (key) =>
    has(store, key)
  )

const persistedFlow: Flow = (name, trigger) =>
  gates(`${name}.persistedGates`, trigger.pipe(switchMap(() => persistStore)), {
    success: (store) => persistedCredsValid(store),
    failure: (store) => !persistedCredsValid(store),
  })

// Auth flow
// The shape of data is the same, whether from signup, login, or persitence.
// Emits: { email, password, accessToken, configToken }

export const authFlow: Flow = (name, trigger) => {
  const next = `${name}.authFlow`

  const persisted = persistedFlow(next, trigger)
  const login = loginFlow(next, loginAction)
  const signup = signupFlow(next, signupAction)

  persisted.failure.subscribe(() => createAuthWindow((win: any) => win.show()))

  const success = merge(login.success, signup.success, persisted.success)
  const failure = merge(login.failure, signup.failure)

  merge(login.success, signup.success).subscribe((creds) =>
    persist(omit(creds, ["password"]))
  )

  return { success, failure }
}

// Container flow
// Creates and assigns a container. Also polls host service and sends
// config token.

const containerFlow: Flow = (name, trigger) => {
  const next = `${name}.containerFlow`

  const create = containerCreateFlow(
    next,
    combineLatest({
      email: trigger.pipe(pluck("email")),
      accessToken: trigger.pipe(pluck("accessToken")),
    })
  )

  const ready = containerPollingFlow(
    next,
    combineLatest({
      containerID: create.success.pipe(pluck("containerID")),
      accessToken: trigger.pipe(pluck("accessToken")),
    })
  )

  const host = hostServiceFlow(
    next,
    combineLatest({
      email: trigger.pipe(pluck("email")),
      accessToken: trigger.pipe(pluck("accessToken")),
    }).pipe(sample(create.success))
  )

  return {
    success: zip(ready.success, host.success).pipe(map(([a]) => a)),
    failure: merge(create.failure, ready.failure, host.failure),
  }
}

// mainFlow
// Composes all other flows together, and is the "entry" point of the
// application.

const mainFlow: Flow = (name, trigger) => {
  const next = `${name}.mainFlow`

  const auth = authFlow(next, trigger)

  const container = containerFlow(next, auth.success)

  const protocol = protocolLaunchFlow(next, auth.success)

  zip(protocol.success, container.success).subscribe(([protocol, info]) => {
    protocolStreamInfo(protocol, info)
  })

  return {
    success: zip(auth.success, container.success, protocol.success),
    failure: merge(auth.failure, container.failure, protocol.failure),
  }
}

// Kick off the main flow to run the app.
mainFlow("app", fromEvent(app, "ready"))

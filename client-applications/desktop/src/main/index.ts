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
import { BehaviorSubject, merge, fromEvent, zip } from "rxjs"
import { switchMapTo, pluck, sample } from "rxjs/operators"
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
import { objectCombine } from "@app/utils/observables"
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
  every(
    ["email", "password", "accessToken", "configToken", "refreshToken"],
    (key) => has(store, key)
  )

const persistedFlow: Flow = (name, trigger) =>
  gates(`${name}.persistedGates`, trigger.pipe(switchMapTo(persistStore)), {
    success: (store) => persistedCredsValid(store),
    failure: (store) => !persistedCredsValid(store),
  })

// Auth flow
// Emits: { email, password, accessToken, configToken}

export const authFlow: Flow = (name, trigger) => {
  const next = `${name}.authFlow`

  const persisted = persistedFlow(next, trigger)
  const login = loginFlow(next, loginAction)
  const signup = signupFlow(next, signupAction)

  persisted.failure.subscribe(() => console.log("success!"))
  persisted.failure.subscribe(() => createAuthWindow((win: any) => win.show()))

  const success = merge(login.success, signup.success, persisted.success)
  const failure = merge(login.failure, signup.failure)

  success.subscribe((creds) => persist(omit(creds, ["password"])))

  return { success, failure }
}

// Container flow

const containerFlow: Flow = (name, trigger) => {
  const next = `${name}.containerFlow`

  const create = containerCreateFlow(
    next,
    objectCombine({
      email: trigger.pipe(pluck("email")),
      accessToken: trigger.pipe(pluck("accessToken")),
    })
  )

  const ready = containerPollingFlow(
    next,
    objectCombine({
      containerID: create.success.pipe(pluck("containerID")),
      accessToken: trigger.pipe(pluck("accessToken")),
    })
  )

  const host = hostServiceFlow(
    next,
    objectCombine({
      email: trigger.pipe(pluck("email")),
      accessToken: trigger.pipe(pluck("email")),
    }).pipe(sample(create.success))
  )

  return {
    success: ready.success.pipe(sample(host.success)),
    failure: merge(create.failure, ready.failure, host.failure),
  }
}

// mainFlow

const mainFlow: Flow = (name, trigger) => {
  const next = `${name}.mainFlow`

  const auth = authFlow(next, trigger)

  const container = containerFlow(next, auth.success)

  const protocol = protocolLaunchFlow(next, auth.success)

  zip([protocol.success, container.success]).subscribe(([protocol, info]) =>
    protocolStreamInfo(protocol, info)
  )

  return {
    success: merge(container.success, protocol.success),
    failure: merge(auth.failure, container.failure, protocol.failure),
  }
}

mainFlow("app", fromEvent(app, "ready"))

// This file is home to the observables that manage the login page in the
// renderer thread. They listen to IPC events what contain loginRequest
// information, and they communicate with the server to authenticate the user.
//
// It's important to note that information received over IPC, like user email,
// or received as a response from the webserver, like an access token, are not
// subscribed directly by observables like userEmail and userAccessToken. All
// received data goes through the full "persistence cycle" first, where it is
// saved to local storage. userEmail and userAccessToken observables then
// "listen" to local storage, and update their values based on local
// storage changes.

import { from, combineLatest } from "rxjs"
import { switchMap, map, pluck } from "rxjs/operators"
import {
  emailLogin,
  emailLoginValid,
  emailLoginError,
  emailLoginConfigToken,
  emailLoginAccessToken,
  emailLoginRefreshToken,
} from "@app/utils/login"
import { loginAction } from "@app/main/events/actions"
import { Flow, gates } from "@app/utils/gates"
import { loadingFrom } from "@app/utils/observables"
import { merge } from "lodash"

const loginGates: Flow = (name, trigger) => {
  const next = `${name}.loginGates`

  const login = gates(
    next,
    loginAction.pipe(
      switchMap(({ email, password }) => from(emailLogin(email, password)))
    ),
    {
      success: (result: any) => emailLoginValid(result),
      failure: (result: any) => emailLoginError(result),
      warning: (result: any) =>
        !emailLoginError(result) && !emailLoginValid(result),
    }
  )

  return {
    ...login,
    loading: loadingFrom(trigger, login.success, login.warning, login.failure),
  }
}

export const loginFlow: Flow = (name, trigger) => {
  const next = `${name}.loginFlow`

  const input = combineLatest({
    email: trigger.pipe(pluck("email")),
    password: trigger.pipe(pluck("password")),
  })

  input.subscribe((v) => console.log("LOGIN INPUT", v))

  const login = loginGates(next, input)

  const configToken = combineLatest([
    login.success,
    input.pipe(pluck("password")),
  ]).pipe(
    switchMap((args) => from(emailLoginConfigToken(...args))),
    map((configToken) => ({ configToken }))
  )

  const tokens = login.success.pipe(
    map((response) => ({
      accessToken: emailLoginAccessToken(response),
      refreshToken: emailLoginRefreshToken(response),
    }))
  )

  const result = combineLatest([input, tokens, configToken]).pipe(
    map(([...args]) => merge(...args))
  )

  return {
    success: result,
    failure: login.failure,
    warning: login.warning,
    loading: loadingFrom(result, login.failure, login.warning),
  }
}

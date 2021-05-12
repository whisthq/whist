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

import { from, combineLatest, Observable } from "rxjs"
import { switchMap, map, pluck } from "rxjs/operators"
import { merge } from "lodash"

import {
  emailLogin,
  emailLoginValid,
  emailLoginError,
  emailLoginConfigToken,
  emailLoginAccessToken,
  emailLoginRefreshToken,
} from "@app/main/flows/auth/flows/login/utils"
import { flow, fork } from "@app/utils/flows"
import { loadingFrom } from "@app/utils/observables"

const loginRequest = flow("loginRequest", (trigger) => {
  const login = fork(
    trigger.pipe(
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
})

const configTokenRequest = flow("configTokenRequest", (trigger) => {
  return {
    success: trigger.pipe(
      switchMap((args: [object, string]) => from(emailLoginConfigToken(...args))),
      map((configToken) => ({ configToken }))
    )
  }
})

const jwtRequest = flow("accessTokenRequest", (trigger) => {
  return {
    success: trigger.pipe(
      map((response) => ({
        accessToken: emailLoginAccessToken(response),
        refreshToken: emailLoginRefreshToken(response),
      }))
    )
  }
})

export default flow("loginFlow", (trigger) => {
  const login = loginRequest(trigger)

  const configToken = configTokenRequest(combineLatest([
    login.success,
    trigger.pipe(pluck("password"))
  ]))

  const jwt = jwtRequest(login.success)

  return {
    success: combineLatest([trigger, jwt, configToken]).pipe(
      map((args: [object, object, object]) => merge(...args))
    ),
    failure: login.failure,
    warning: login.warning,
    loading: login.loading,
  }
})

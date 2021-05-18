// This file is home to the observables that manage the signup page in the
// renderer thread. They listen to IPC events what contain signupRequest
// information, and they communicate with the server to authenticate the user.
//
// It's important to note that information received over IPC, like user email,
// or received as a response from the webserver, like an access token, are not
// subscribed directly by observables like userEmail and userConfigToken. All
// received data goes through the full "persistence cycle" first, where it is
// saved to local storage. userEmail and userConfigToken observables then
// "listen" to local storage, and update their values based on local
// storage changes.

import { from, combineLatest, zip } from "rxjs"
import { switchMap, map, pluck } from "rxjs/operators"
import {
  emailSignup,
  emailSignupValid,
  emailSignupError,
  emailSignupAccessToken,
  emailSignupRefreshToken,
} from "@app/utils/signup"
import { createConfigToken, encryptConfigToken } from "@app/utils/crypto"
import { loadingFrom } from "@app/utils/observables"
import { flow, fork } from "@app/utils/flows"

const signupGates = flow<any>("signupGates", (trigger) =>
  fork(
    trigger.pipe(
      switchMap(({ email, password, configToken }) =>
        from(emailSignup(email, password, configToken))
      )
    ),
    {
      success: (result: any) => emailSignupValid(result),
      failure: (result: any) => emailSignupError(result),
      warning: (result: any) =>
        !emailSignupError(result) && !emailSignupError(result),
    }
  )
)

export const generateConfigTokenGate = flow(
  "generateConfigTokenGate",
  (trigger) =>
    fork(
      trigger.pipe(
        switchMap(({ password }) =>
          from(
            createConfigToken().then(
              async (token) => await encryptConfigToken(token, password)
            )
          )
        )
      ),
      {
        success: () => true,
      }
    )
)

export const signupFlow = flow("signupFlow", (trigger) => {
  const input = combineLatest({
    email: trigger.pipe(pluck("email")),
    password: trigger.pipe(pluck("password")),
    configToken: generateConfigTokenGate(trigger).success,
  })

  const signup = signupGates(input)

  const tokens = signup.success.pipe(
    map((response) => ({
      accessToken: emailSignupAccessToken(response),
      refreshToken: emailSignupRefreshToken(response),
    }))
  )

  const result = zip([input, tokens]).pipe(map(([...args]) => ({ ...args })))
  return {
    success: result,
    failure: signup.failure,
    warning: signup.warning,
    loading: loadingFrom(trigger, result, signup.failure, signup.warning),
  }
})

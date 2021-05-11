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

import { from, merge } from "rxjs"
import {
  emailSignup,
  emailSignupValid,
  emailSignupError,
} from "@app/utils/signup"
import { createConfigToken, encryptConfigToken } from "@app/utils/crypto"
import { switchMap, map, withLatestFrom } from "rxjs/operators"
import { loadingFrom } from "@app/utils/observables"
import { Flow, gates } from "@app/utils/gates"

const signupConfigTokenGate: Flow = (name, trigger) =>
  gates(
    `${name}.configTokenGate`,
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

export const signupGates: Flow = (name, trigger) =>
  gates(
    `${name}.signupGates`,
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

export const signupFlow: Flow = (name, trigger) => {
  const nextName = `${name}.signupFlow`

  const config = signupConfigTokenGate(nextName, trigger)

  const signup = signupGates(
    nextName,
    trigger.pipe(
      withLatestFrom(config.success),
      map(([{ email, password }, configToken]) => ({
        email,
        password,
        configToken,
      }))
    )
  )

  const success = signup.success
  const failure = merge(config.failure, signup.failure)
  const loading = loadingFrom(trigger, success, failure)

  return { success, failure, loading }
}

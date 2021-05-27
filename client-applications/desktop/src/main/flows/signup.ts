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

import { from, zip, Observable } from "rxjs"
import { switchMap, map, withLatestFrom } from "rxjs/operators"
import {
  emailSignup,
  emailSignupValid,
  emailSignupError,
  emailSignupAccessToken,
  emailSignupRefreshToken,
} from "@app/utils/signup"
import { createConfigToken, encryptConfigToken } from "@app/utils/crypto"
import { loadingFrom } from "@app/utils/observables"
import { flow, fork, createTrigger } from "@app/utils/flows"
import { merge } from "lodash"

const signupRequest = flow<any>("signupRequest", (trigger) =>
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
        !emailSignupError(result) && !emailSignupValid(result),
    }
  )
)

const generateConfigToken = flow<any>("generateConfigToken", (trigger) =>
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

export default flow(
  "signupFlow",
  (trigger: Observable<{ email: string; password: string }>) => {
    const config = generateConfigToken(trigger)

    const input = zip([trigger, config.success]).pipe(
      map(([{ email, password }, configToken]) => ({
        email,
        password,
        configToken,
      }))
    )

    const signup = signupRequest(input)

    const tokens = signup.success.pipe(
      map((response) => ({
        accessToken: emailSignupAccessToken(response),
        refreshToken: emailSignupRefreshToken(response),
      }))
    )

    const result = tokens.pipe(
      withLatestFrom(input),
      map(([...args]) => merge(...args))
    )

    return {
      success: createTrigger("signupFlowSuccess", result),
      failure: createTrigger("signupFlowFailure", signup.failure),
      warning: createTrigger("signupFlowWarning", signup.warning),
      loading: createTrigger(
        "signupFlowLoading",
        loadingFrom(trigger, result, signup.failure, signup.warning)
      ),
    }
  }
)

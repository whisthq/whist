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

import { from, combineLatest } from "rxjs"
import {
  emailSignup,
  emailSignupValid,
  emailSignupError,
} from "@app/utils/signup"
import { createConfigToken, encryptConfigToken } from "@app/utils/crypto"
import { switchMap } from "rxjs/operators"
import { signupAction } from "@app/main/events/actions"
import { gates } from "@app/utils/gates"
import { loadingFrom } from "@app/utils/observables"

export const { success: signupConfigSuccess } = gates(
  `signupConfig`,
  signupAction.pipe(
    switchMap((req) =>
      from(
        createConfigToken().then(
          async (token) => await encryptConfigToken(token, req.password)
        )
      )
    )
  ),
  {
    success: () => true,
  }
)

export const {
  success: signupSuccess,
  failure: signupFailure,
  warning: signupWarning,
} = gates(
  `signup`,
  combineLatest([signupAction, signupConfigSuccess]).pipe(
    switchMap(([req, token]) =>
      from(emailSignup(req.email, req.password, token))
    )
  ),
  {
    success: (result: any) => emailSignupValid(result),
    failure: (result: any) => emailSignupError(result),
    warning: (result: any) =>
      !emailSignupError(result) && !emailSignupError(result),
  }
)

export const signupLoading = loadingFrom(
  signupAction,
  signupFailure,
  signupWarning
)

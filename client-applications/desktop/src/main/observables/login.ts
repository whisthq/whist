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

import { from } from "rxjs"
import { switchMap } from "rxjs/operators"
import { emailLogin, emailLoginValid, emailLoginError } from "@app/utils/login"
import { loginAction } from "@app/main/events/actions"
import { gates } from "@app/utils/gates"
import { loadingFrom } from "@app/utils/observables"

export const {
  success: loginSuccess,
  failure: loginFailure,
  warning: loginWarning,
} = gates(
  `login`,
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

export const loginLoading = loadingFrom(loginAction, loginFailure, loginWarning)

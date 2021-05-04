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
import { emailLogin, emailLoginValid, emailLoginError } from "@app/utils/login"
import { formatLogin } from "@app/utils/formatters"
import { loginAction } from "@app/main/events/actions"
import { factory } from "@app/utils/observables"

export const {
  request: loginRequest,
  process: loginProcess,
  success: loginSuccess,
  failure: loginFailure,
  warning: loginWarning,
  loading: loginLoading,
} = factory("login", {
  request: loginAction,
  process: ({ email, password }) => from(emailLogin(email, password)),
  success: (result) => emailLoginValid(result),
  failure: (result) => emailLoginError(result),
  warning: (result) => !emailLoginError(result) && !emailLoginValid(result),
  logging: {
    request: ["hiding password", ({ email }) => [email, "********"]],
    success: ["only { json } tokens", formatLogin],
  },
})

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

import { fromEventIPC } from '@app/main/events/ipc'
import { debugObservables } from '@app/utils/logging'
import { share } from 'rxjs/operators'

// export const loginRequest = fromEventIPC("loginRequest").pipe(
//   filter((req) => (req?.email ?? "") !== "" && (req?.password ?? "") !== ""),
//   map(({ email, password }) => [email, password]),
//   share()
// )

export const signoutProcess = fromEventIPC('signoutRequest').pipe(share())

// Logging

debugObservables([signoutProcess, 'signoutProcess'])

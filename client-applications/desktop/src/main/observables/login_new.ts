// This file is home to the observables that process authentication state, specifically:
// access and refresh tokens.

// Note that authentication and refresh tokens go through the "persistence cycle" before these observables
// fire.

import { fromEventIPC } from "@app/main/events/ipc"
import { fromEventPersist } from "@app/main/events/persist"
import { from } from "rxjs"
import { loadingFrom } from "@app/utils/observables"
import {
  debugObservables,
  warningObservables,
  errorObservables,
} from "@app/utils/logging"
import { emailLogin, emailLoginValid, emailLoginError } from "@app/utils/login"
import { filter, tap, map, share, exhaustMap } from "rxjs/operators"

export const accessToken = fromEventPersist("accessToken").pipe(
  filter((token: string) => !!token),
  tap(val => console.log(`ACCESS TOKEN OBSERVABLE ${val}`))
)

export const refreshToken = fromEventPersist("refreshToken").pipe(
  filter((token: string) => !!token),
  tap(val => console.log(`REFRESH TOKEN OBSERVABLE ${val}`))
)

// TODO: remove subscribes eventually, they're for debugging
accessToken.subscribe()
refreshToken.subscribe()
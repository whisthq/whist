// This file is home to observables of user data. This data should be sourced
// directly from persisted storage, which is the "beginning" of the reactive
// data flow in the application.
//
// It may be tempting to pull user data from other observables in the
// application, such as getting the userAccessToken directly from
// loginSuccess, but that can create a circular dependency.
//
// Instead, it's expected that data returned from observables like loginSuccess
// or signupSuccess will go through the "persistence cycle" and be first saved
// to local storage. The observables in this file then "listen" to local storage
// to receive the updated data.

import { fromEventPersist } from "@app/main/events/events"
import { map } from "rxjs/operators"

export const userEmail = fromEventPersist("userEmail").pipe(
    map((s) => s as string)
)
export const userConfigToken = fromEventPersist("userConfigToken").pipe(
    map((s) => s as string)
)
export const userAccessToken = fromEventPersist("userAccessToken").pipe(
    map((s) => s as string)
)

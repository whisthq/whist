// This file is home to observables that manage mandelbox creation.
// Their responsibilities are to listen for state that will trigger protocol
// launches. These observables then communicate with the webserver and
// poll the state of the mandelboxs while they spin up.
//
// These observables are subscribed by protocol launching observables, which
// react to success mandelbox creation emissions from here.

import { from, of, timer } from "rxjs"
import {
  map,
  switchMap,
  retryWhen,
  delayWhen,
  catchError,
} from "rxjs/operators"

import {
  mandelboxRequest,
  mandelboxCreateSuccess,
} from "@app/main/utils/mandelbox"
import { fork, flow } from "@app/main/utils/flows"
import { AWSRegion } from "@app/@types/aws"

export default flow<{
  accessToken: string
  userEmail: string
  regions: AWSRegion[]
}>("mandelboxCreateFlow", (trigger) => {
  let attempts = 0

  const create = fork(
    trigger.pipe(
      switchMap((t) =>
        t.regions.length > 0
          ? from(mandelboxRequest(t.accessToken, t.regions, t.userEmail))
          : of({})
      ),
      map((req) => {
        if (!mandelboxCreateSuccess(req)) throw new Error()
        return req
      }),
      retryWhen((errors) =>
        errors.pipe(
          delayWhen(() => {
            if (attempts < 10) {
              attempts = attempts + 1
              return timer(1000)
            }
            throw new Error()
          })
        )
      ),
      catchError((error) => of(error)) // We retry because sometimes Whist launches before the computer connects to Wifi
    ),
    {
      success: (req) => mandelboxCreateSuccess(req),
      failure: (req) => !mandelboxCreateSuccess(req),
    }
  )

  return {
    success: create.success.pipe(
      map((response: { json: { mandelbox_id: string; ip: string } }) => ({
        mandelboxID: response.json.mandelbox_id,
        ip: response.json.ip,
      }))
    ),
    failure: create.failure,
  }
})

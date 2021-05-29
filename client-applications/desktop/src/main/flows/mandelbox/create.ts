// This file is home to observables that manage mandelbox creation.
// Their responsibilities are to listen for state that will trigger protocol
// launches. These observables then communicate with the webserver and
// poll the state of the mandelboxs while they spin up.
//
// These observables are subscribed by protocol launching observables, which
// react to success mandelbox creation emissions from here.

import { from } from "rxjs"
import { map, switchMap } from "rxjs/operators"

import { mandelboxCreate, mandelboxCreateSuccess } from "@app/utils/mandelbox"
import { fork, flow } from "@app/utils/flows"

export default flow<{ sub: string; accessToken: string }>(
  "mandelboxCreateFlow",
  (trigger) => {
    const create = fork(
      trigger.pipe(
        switchMap(({ sub, accessToken }) =>
          from(mandelboxCreate(sub, accessToken))
        )
      ),
      {
        success: (req) => mandelboxCreateSuccess(req),
        failure: (req) => !mandelboxCreateSuccess(req),
      }
    )

    return {
      success: create.success.pipe(
        map((response: { json: { ID: string } }) => ({
          mandelboxID: response.json.ID,
        }))
      ),
      failure: create.failure,
    }
  }
)

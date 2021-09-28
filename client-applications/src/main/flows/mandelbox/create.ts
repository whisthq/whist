// This file is home to observables that manage mandelbox creation.
// Their responsibilities are to listen for state that will trigger protocol
// launches. These observables then communicate with the webserver and
// poll the state of the mandelboxs while they spin up.
//
// These observables are subscribed by protocol launching observables, which
// react to success mandelbox creation emissions from here.

import { from, of, merge, timer } from "rxjs"
import {
  map,
  switchMap,
  withLatestFrom,
  retryWhen,
  delayWhen,
  catchError,
} from "rxjs/operators"

import { mandelboxRequest, mandelboxCreateSuccess } from "@app/utils/mandelbox"
import { sortRegionByProximity } from "@app/utils/region"
import { fork, flow } from "@app/utils/flows"
import { defaultAllowedRegions } from "@app/utils/constants"
import { AWSRegion } from "@app/@types/aws"

export default flow<{
  accessToken: string
}>("mandelboxCreateFlow", (trigger) => {
  let attempts = 0

  const region = fork(
    trigger.pipe(
      switchMap(({ accessToken }) =>
        from(sortRegionByProximity(defaultAllowedRegions)).pipe(
          withLatestFrom(of(accessToken))
        )
      ),
      map(([regions, accessToken]: [AWSRegion[], string]) => {
        if (regions.length === 0) throw new Error()
        return [regions, accessToken]
      }),
      retryWhen((errors) =>
        errors.pipe(
          delayWhen(() => {
            if (attempts < 20) {
              attempts = attempts + 1
              return timer(500)
            }
            throw new Error()
          })
        )
      ),
      catchError((error) => of(error))
    ),
    {
      success: (regions) => regions.length > 0,
      failure: (regions) => !(regions.length > 0),
    }
  )

  const create = fork(
    region.success.pipe(
      switchMap(([regions, accessToken]: [AWSRegion[], string]) =>
        regions.length > 0
          ? from(mandelboxRequest(accessToken, regions))
          : of({})
      )
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
    failure: merge(region.failure, create.failure),
  }
})

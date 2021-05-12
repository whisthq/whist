// This file is home to observables that manage container creation.
// Their responsibilities are to listen for state that will trigger protocol
// launches. These observables then communicate with the webserver and
// poll the state of the containers while they spin up.
//
// These observables are subscribed by protocol launching observables, which
// react to success container creation emissions from here.

import { from, of, interval, merge } from "rxjs"
import { map, takeUntil, switchMap, take, mapTo, share } from "rxjs/operators"
import { some } from "lodash"

import {
  containerInfo,
  containerInfoIP,
  containerInfoPorts,
  containerInfoSecretKey,
  containerInfoError,
  containerInfoSuccess,
  containerInfoPending,
} from "@app/main/flows/container/flows/polling/utils"
import { loadingFrom } from "@app/utils/observables"
import { fork, flow } from "@app/utils/flows"

const containerInfoRequest = flow("containerInfoRequest", (_, trigger) =>
  fork(
    trigger.pipe(
      switchMap(({ containerID, accessToken }) =>
        from(containerInfo(containerID, accessToken))
      )
    ),
    {
      success: (result) => containerInfoSuccess(result),
      pending: (result) => containerInfoPending(result),
      failure: (result) =>
        containerInfoError(result) ||
        !some([containerInfoPending(result), containerInfoSuccess(result)]),
    }
  )
)

const containerPollingInner = flow("containerPollingInner", (name, trigger) => {
  const tick = trigger.pipe(
    switchMap((args) => interval(1000).pipe(mapTo(args)))
  )
  const poll = containerInfoRequest(name, tick)

  return {
    pending: poll.pending.pipe(takeUntil(merge(poll.success, poll.failure))),
    success: poll.success.pipe(take(1)),
    failure: poll.failure.pipe(take(1)),
  }
})

export default flow(
  "containerPollingFlow",
  (name, trigger) => {
    const poll = trigger.pipe(
      map((args) => containerPollingInner(name, of(args))),
      share()
    )

    const success = poll.pipe(switchMap((inner) => inner.success))
    const failure = poll.pipe(switchMap((inner) => inner.failure))
    const pending = poll.pipe(switchMap((inner) => inner.pending))
    const loading = loadingFrom(trigger, success, failure)

    // We probably won't subscribe to "pending" in the rest of the app,
    // so we subscribe to it deliberately here. If it has no subscribers,
    // it won't emit. We would like for it to emit for logging purposes.
    // We must remeber to takeUntil so it stops when we're finished.
    pending.pipe(takeUntil(merge(success, failure))).subscribe()

    return {
      success: success.pipe(
        map((response) => ({
          containerIP: containerInfoIP(response),
          containerSecret: containerInfoSecretKey(response),
          containerPorts: containerInfoPorts(response),
        }))
      ),
      failure,
      pending,
      loading,
    }
  }
)

// This file is home to observables that manage container creation.
// Their responsibilities are to listen for state that will trigger protocol
// launches. These observables then communicate with the webserver and
// poll the state of the containers while they spin up.
//
// These observables are subscribed by protocol launching observables, which
// react to success container creation emissions from here.

import {
  containerCreate,
  containerInfo,
  containerInfoError,
  containerInfoSuccess,
  containerInfoPending,
} from "@app/utils/container"
import {
  userEmail,
  userAccessToken,
  userConfigToken,
} from "@app/main/observables/user"
import { eventUpdateNotAvailable } from "@app/main/events/autoupdate"
import { loadingFrom } from "@app/utils/observables"
import { from, of, zip, combineLatest, interval, merge } from "rxjs"
import {
  map,
  takeUntil,
  switchMap,
  take,
  mapTo,
  withLatestFrom,
  tap,
  share,
} from "rxjs/operators"
import { gates, Flow } from "@app/utils/gates"
import { some } from "lodash"

const containerCreateGates: Flow = (name, trigger) =>
  gates(
    name,
    trigger.pipe(
      switchMap(([email, token]) => {
        return from(containerCreate(email, token))
      })
    ),
    {
      success: (req: { json: { ID: string } }) => (req?.json?.ID ?? "") !== "",
      failure: (req: { json: { ID: string } }) => (req?.json?.ID ?? "") === "",
    }
  )

const containerInfoGates: Flow = (name, trigger) =>
  gates(
    name,
    trigger.pipe(switchMap(([id, token]) => from(containerInfo(id, token)))),
    {
      success: (result) => containerInfoSuccess(result),
      pending: (result) => containerInfoPending(result),
      failure: (result) =>
        containerInfoError(result) ||
        !some([containerInfoPending(result), containerInfoSuccess(result)]),
    }
  )

const containerPollingInner: Flow = (name, trigger) => {
  const tick = trigger.pipe(
    switchMap((args) => interval(1000).pipe(mapTo(args)))
  )
  const poll = containerInfoGates(name, tick)

  return {
    pending: poll.pending.pipe(takeUntil(merge(poll.success, poll.failure))),
    success: poll.success.pipe(take(1)),
    failure: poll.failure.pipe(take(1)),
  }
}

const containerPollingFlow: Flow = (name, trigger) => {
  const poll = trigger.pipe(
    map((args) => containerPollingInner(name, of(args))),
    share()
  )

  const success = poll.pipe(switchMap((inner) => inner.success))
  const failure = poll.pipe(switchMap((inner) => inner.failure))
  const pending = poll.pipe(switchMap((inner) => inner.pending))
  const loading = loadingFrom(trigger, success, failure)

  return { success, failure, pending, loading }
}

export const {
  success: containerCreateSuccess,
  failure: containerCreateFailure,
  warning: containerCreateWarning,
} = containerCreateGates(
  `containerCreate`,
  combineLatest([
    zip(userEmail, userAccessToken, userConfigToken),
    eventUpdateNotAvailable,
  ]).pipe(map(([args]) => args))
)

export const {
  success: containerPollingSuccess,
  failure: containerPollingFailure,
  loading: containerPollingLoading,
  pending: containerPollingPending,
} = containerPollingFlow(
  `containerPolling`,
  containerCreateSuccess.pipe(
    map((response) => response.json.ID),
    withLatestFrom(userAccessToken)
  )
)
containerPollingPending.subscribe()

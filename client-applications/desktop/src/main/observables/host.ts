import {
  userEmail,
  userAccessToken,
  userConfigToken,
} from "@app/main/observables/user"
import { containerCreateSuccess } from "@app/main/observables/container"
import { loadingFrom } from "@app/utils/observables"
import {
  hostServiceInfo,
  hostServiceInfoValid,
  hostServiceInfoIP,
  hostServiceInfoPort,
  hostServiceInfoPending,
  hostServiceInfoSecret,
  hostServiceConfig,
  hostServiceConfigValid,
  hostServiceConfigError,
} from "@app/utils/host"
import { from, interval, of, merge } from "rxjs"
import {
  map,
  share,
  takeUntil,
  switchMap,
  withLatestFrom,
  mapTo,
  take,
} from "rxjs/operators"
import { gates, Flow } from "@app/utils/gates"
import { some } from "lodash"

const hostServiceInfoGates: Flow = (name, trigger) =>
  gates(
    name,
    trigger.pipe(
      switchMap(([email, token]) => from(hostServiceInfo(email, token)))
    ),
    {
      success: (result) => hostServiceInfoValid(result),
      pending: (result) => hostServiceInfoPending(result),
      failure: (result) =>
        !some([hostServiceInfoValid(result), hostServiceInfoPending]),
    }
  )

const hostPollingInner: Flow = (name, trigger) => {
  const tick = trigger.pipe(
    switchMap((args) => interval(1000).pipe(mapTo(args)))
  )
  const poll = hostServiceInfoGates(name, tick)

  return {
    pending: poll.pending.pipe(takeUntil(merge(poll.success, poll.failure))),
    success: poll.success.pipe(take(1)),
    failure: poll.failure.pipe(take(1)),
  }
}

const hostInfoFlow: Flow = (name, trigger) => {
  const poll = trigger.pipe(
    map((args) => hostPollingInner(name, of(args))),
    share()
  )

  const success = poll.pipe(switchMap((inner) => inner.success))
  const failure = poll.pipe(switchMap((inner) => inner.failure))
  const pending = poll.pipe(switchMap((inner) => inner.pending))
  const loading = loadingFrom(trigger, success, failure)

  return { success, failure, pending, loading }
}

const hostConfigGates: Flow = (name, trigger) =>
  gates(
    name,
    trigger.pipe(
      switchMap(([ip, port, secret, email, token]) =>
        from(hostServiceConfig(ip, port, secret, email, token))
      )
    ),
    {
      success: (result) => hostServiceConfigValid(result),
      failure: (result) => hostServiceConfigError(result),
    }
  )

export const {
  success: hostInfoSuccess,
  failure: hostInfoFailure,
  pending: hostInfoPending,
} = hostInfoFlow(
  "hostInfo",
  containerCreateSuccess.pipe(
    withLatestFrom(userEmail, userAccessToken),
    map(([_, email, token]) => [email, token])
  )
)

export const {
  success: hostConfigSuccess,
  failure: hostConfigFailure,
} = hostConfigGates(
  "hostConfig",
  hostInfoSuccess.pipe(
    map((res) => [
      hostServiceInfoIP(res),
      hostServiceInfoPort(res),
      hostServiceInfoSecret(res),
    ]),
    withLatestFrom(userEmail, userConfigToken),
    map(([[ip, port, secret], email, token]) => [
      ip,
      port,
      secret,
      email,
      token,
    ])
  )
)

hostInfoPending.subscribe()

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
  HostServiceInfoResponse,
  HostServiceConfigResponse,
} from "@app/main/utils/host"
import { from, interval, of, merge, combineLatest, Observable } from "rxjs"
import {
  map,
  share,
  takeUntil,
  switchMap,
  mapTo,
  take,
  pluck,
} from "rxjs/operators"
import { flow, fork } from "@app/main/utils/flows"
import { some } from "lodash"

const hostServiceInfoGates = flow<{ email: string; accessToken: string }>(
  "hostServiceInfoGates",
  (trigger) =>
    fork(
      trigger.pipe(
        switchMap(({ email, accessToken }) =>
          from(hostServiceInfo(email, accessToken))
        )
      ),
      {
        success: (result: HostServiceInfoResponse) =>
          hostServiceInfoValid(result),
        pending: (result: HostServiceInfoResponse) =>
          hostServiceInfoPending(result),
        failure: (result: HostServiceInfoResponse) =>
          !some([hostServiceInfoValid(result), hostServiceInfoPending(result)]),
      }
    )
)

const hostPollingInner = flow("hostPollingInner", (trigger) => {
  const tick = trigger.pipe(
    switchMap((args) =>
      interval(1000).pipe(mapTo(args))
    )
  )
  const poll = hostServiceInfoGates(tick)

  return {
    pending: poll.pending.pipe(takeUntil(merge(poll.success, poll.failure))),
    success: poll.success.pipe(take(1)),
    failure: poll.failure.pipe(take(1)),
  }
})

const hostInfoFlow = flow("hostInfoFlow", (trigger) => {
  const poll = trigger.pipe(
    map((args) => hostPollingInner(of(args))),
    share()
  )

  const success = poll.pipe(switchMap((inner) => inner.success))
  const failure = poll.pipe(switchMap((inner) => inner.failure))
  const pending = poll.pipe(switchMap((inner) => inner.pending))
  const loading = loadingFrom(trigger, success, failure)

  return {
    success: success.pipe(
      map((response) => ({
        containerIP: hostServiceInfoIP(response),
        containerPort: hostServiceInfoPort(response),
        contianerSecret: hostServiceInfoSecret(response),
      }))
    ),
    failure,
    pending,
    loading,
  }
})

const hostConfigFlow = flow<{
  hostIP: string
  hostPort: number
  hostSecret: string
  email: string
  configToken: string
}>("hostConfigFlow", (trigger) =>
  fork(
    trigger.pipe(
      switchMap(({ hostIP, hostPort, hostSecret, email, configToken }) =>
        from(
          hostServiceConfig(hostIP, hostPort, hostSecret, email, configToken)
        )
      )
    ),
    {
      success: (result: HostServiceConfigResponse) =>
        hostServiceConfigValid(result),
      failure: (result: HostServiceConfigResponse) =>
        hostServiceConfigError(result),
    }
  )
)

export default flow("hostServiceFlow", (trigger) => {
  const info = hostInfoFlow(trigger)

  const config = hostConfigFlow(
    combineLatest({
      email: trigger.pipe(pluck("email")) as Observable<string>,
      configToken: trigger.pipe(pluck("configToken")) as Observable<string>,
      hostIP: info.success.pipe(pluck("containerIP")) as Observable<string>,
      hostPort: info.success.pipe(pluck("containerPort")) as Observable<number>,
      hostSecret: info.success.pipe(
        pluck("containerSecret")
      ) as Observable<string>,
    })
  )

  return {
    success: config.success,
    failure: merge(info.failure, config.failure),
  }
})

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
} from "@app/utils/host"
import { from, interval, of, merge, zip } from "rxjs"
import { map, share, takeUntil, switchMap, mapTo, take } from "rxjs/operators"
import { flow, fork } from "@app/utils/flows"
import { some, pick } from "lodash"

const hostServiceInfoRequest = flow<{
  sub: string
  accessToken: string
  configToken: string
}>("hostServiceInfoRequest", (trigger) =>
  fork(
    trigger.pipe(
      switchMap(({ sub, accessToken }) =>
        from(hostServiceInfo(sub, accessToken))
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

const hostPollingInner = flow<{
  sub: string
  accessToken: string
  configToken: string
}>("hostPollingInner", (trigger) => {
  const tick = trigger.pipe(
    switchMap((args) => interval(1000).pipe(mapTo(args)))
  )
  const poll = hostServiceInfoRequest(tick)

  return {
    pending: poll.pending.pipe(takeUntil(merge(poll.success, poll.failure))),
    success: poll.success.pipe(take(1)),
    failure: poll.failure.pipe(takeUntil(poll.success), take(1)),
  }
})

const hostInfoFlow = flow<{
  sub: string
  accessToken: string
  configToken: string
}>("hostInfoFlow", (trigger) => {
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
        mandelboxIP: hostServiceInfoIP(response),
        mandelboxPort: hostServiceInfoPort(response),
        mandelboxSecret: hostServiceInfoSecret(response),
      }))
    ),
    failure,
    pending,
    loading,
  }
})

const hostConfigFlow = flow<{
  mandelboxIP: string
  mandelboxPort: number
  mandelboxSecret: string
  sub: string
  configToken: string
}>("hostConfigFlow", (trigger) =>
  fork(
    trigger.pipe(
      switchMap(
        ({ mandelboxIP, mandelboxPort, mandelboxSecret, sub, configToken }) =>
          from(
            hostServiceConfig(
              mandelboxIP,
              mandelboxPort,
              mandelboxSecret,
              sub,
              configToken
            )
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

export default flow<{
  sub: string
  accessToken: string
  configToken: string
}>("hostServiceFlow", (trigger) => {
  const info = hostInfoFlow(trigger)

  const config = hostConfigFlow(
    zip(trigger, info.success).pipe(
      map(([t, i]) => ({
        ...pick(t, ["sub", "configToken"]),
        ...pick(i, ["mandelboxIP", "mandelboxPort", "mandelboxSecret"]),
      }))
    )
  )

  config.success.subscribe((x: any) => console.log("HOST CONFIG FLOW", x))

  return {
    success: config.success,
    failure: merge(info.failure, config.failure),
  }
})

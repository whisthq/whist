import { combineLatest, merge, Observable } from "rxjs"
import { pluck } from "rxjs/operators"

import containerCreateFlow from "@app/main/flows/container/create"
import containerPollingFlow from "@app/main/flows/container/polling"
import hostServiceFlow from "@app/main/flows/container/host"
import { flow, createTrigger } from "@app/utils/flows"
import { fromSignal } from "@app/utils/observables"
import { AWSRegion } from "@app/@types/aws"

export default flow("containerFlow", (trigger) => {
  const create = containerCreateFlow(
    combineLatest({
      email: trigger.pipe(pluck("email")) as Observable<string>,
      accessToken: trigger.pipe(pluck("accessToken")) as Observable<string>,
      region: trigger.pipe(pluck("region")) as Observable<AWSRegion>,
    })
  )

  const polling = containerPollingFlow(
    combineLatest({
      containerID: create.success.pipe(
        pluck("containerID")
      ) as Observable<string>,
      accessToken: trigger.pipe(pluck("accessToken")) as Observable<string>,
    })
  )

  const host = hostServiceFlow(
    fromSignal(
      combineLatest({
        email: trigger.pipe(pluck("email")) as Observable<string>,
        accessToken: trigger.pipe(pluck("accessToken")) as Observable<string>,
        configToken: trigger.pipe(pluck("configToken")) as Observable<string>,
      }),
      create.success
    )
  )

  return {
    success: createTrigger("containerFlowSuccess", polling.success),
    failure: createTrigger(
      "containerFlowFailure",
      merge(create.failure, polling.failure, host.failure)
    ),
  }
})

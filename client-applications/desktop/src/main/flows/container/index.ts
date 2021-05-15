import { combineLatest, merge, zip, Observable } from "rxjs"
import { pluck, sample, map } from "rxjs/operators"

import containerCreateFlow from "@app/main/flows/container/flows/create"
import containerPollingFlow from "@app/main/flows/container/flows/polling"
import hostServiceFlow from "@app/main/flows/container/flows/host"
import { flow, createTrigger } from "@app/main/utils/flows"

export default flow("containerFlow", (trigger) => {
  const create = containerCreateFlow(
    combineLatest({
      email: trigger.pipe(pluck("email")) as Observable<string>,
      accessToken: trigger.pipe(pluck("accessToken")) as Observable<string>,
    })
  )

  const ready = containerPollingFlow(
    combineLatest({
      containerID: create.success.pipe(pluck("containerID")) as Observable<string>,
      accessToken: trigger.pipe(pluck("accessToken")) as Observable<string>,
    })
  )

  const host = hostServiceFlow(
    combineLatest({
      email: trigger.pipe(pluck("email")),
      accessToken: trigger.pipe(pluck("accessToken")),
    }).pipe(sample(create.success))
  )

  return {
    success: createTrigger(
      "containerFlowSuccess",
      zip(ready.success, host.success).pipe(map(([a]) => a))
    ),
    failure: createTrigger(
      "containerFlowFailure",
      merge(create.failure, ready.failure, host.failure)
    ),
  }
})

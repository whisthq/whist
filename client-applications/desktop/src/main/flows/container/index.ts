import { combineLatest, merge, zip, Observable } from "rxjs"
import { pluck, sample, map } from "rxjs/operators"

import containerCreateFlow from "@app/main/flows/container/create"
import containerPollingFlow from "@app/main/flows/container/polling"
import hostServiceFlow from "@app/main/flows/container/host"
import { flow, createTrigger } from "@app/utils/flows"

export default flow("containerFlow", (trigger) => {
  const create = containerCreateFlow(
    combineLatest({
      email: trigger.pipe(pluck("email")) as Observable<string>,
      accessToken: trigger.pipe(pluck("accessToken")) as Observable<string>,
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
    combineLatest({
      email: trigger.pipe(pluck("email")),
      accessToken: trigger.pipe(pluck("accessToken")),
    }).pipe(sample(create.success))
  )

  return {
    success: createTrigger(
      "containerFlowSuccess",
      polling.success
    ),
    failure: createTrigger(
      "containerFlowFailure",
      merge(create.failure, polling.failure, host.failure)
    ),
  }
})

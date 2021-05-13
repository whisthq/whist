import { combineLatest, merge, zip, Observable } from "rxjs"
import { pluck, sample, map } from "rxjs/operators"

import containerCreateFlow from "@app/main/flows/container/flows/create"
import containerPollingFlow from "@app/main/flows/container/flows/polling"
import hostServiceFlow from "@app/main/flows/container/flows/host"
import { flow, fromTrigger } from "@app/main/utils/flows"

export default flow(
  "containerFlow",
  (trigger) => {
    const create = containerCreateFlow(
      combineLatest({
        email: trigger.pipe(pluck("email")),
        accessToken: trigger.pipe(pluck("accessToken")),
      })
    )

    const ready = containerPollingFlow(
      combineLatest({
        containerID: fromTrigger("containerCreateFlowSuccess").pipe(
          pluck("containerID")
        ),
        accessToken: trigger.pipe(pluck("accessToken")),
      } as Observable<Record<string, string>>)
    )

    const host = hostServiceFlow(
      combineLatest({
        email: trigger.pipe(pluck("email")),
        accessToken: trigger.pipe(pluck("accessToken")),
      }).pipe(sample(fromTrigger("containerCreateFlowSuccess")))
    )

    return {
      success: zip(ready.success, host.success).pipe(map(([a]) => a)),
      failure: merge(create.failure, ready.failure, host.failure),
    }
  },
  true
)

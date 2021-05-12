import { combineLatest, zip, merge } from "rxjs"
import { pluck, sample } from "rxjs/operators"

import containerCreateFlow from "@app/main/flows/container/flows/create"
import containerPollingFlow from "@app/main/flows/container/flows/polling"
import hostServiceFlow from "@app/main/flows/container/flows/host"
import { flow } from "@app/utils/flows"


export default flow("containerFlow", (name, trigger) => {
  const create = containerCreateFlow(
    name,
    combineLatest({
      email: trigger.pipe(pluck("email")),
      accessToken: trigger.pipe(pluck("accessToken")),
    })
  )

  const ready = containerPollingFlow(
    name,
    combineLatest({
      containerID: create.success.pipe(pluck("containerID")),
      accessToken: trigger.pipe(pluck("accessToken")),
    })
  )

  const host = hostServiceFlow(
    name,
    combineLatest({
      email: trigger.pipe(pluck("email")),
      accessToken: trigger.pipe(pluck("accessToken")),
    }).pipe(sample(create.success))
  )

  return {
    success: zip(ready.success, host.success).pipe(map(([a]) => a)),
    failure: merge(create.failure, ready.failure, host.failure),
  }
})

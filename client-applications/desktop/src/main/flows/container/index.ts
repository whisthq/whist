import { merge, Observable, zip } from "rxjs"
import { map } from "rxjs/operators"

import containerCreateFlow from "@app/main/flows/container/create"
import containerPollingFlow from "@app/main/flows/container/polling"
import hostServiceFlow from "@app/main/flows/container/host"
import { flow, createTrigger } from "@app/utils/flows"
import { pick } from "lodash"

export default flow(
  "containerFlow",
  (
    trigger: Observable<{
      sub: string
      accessToken: string
      configToken: string
    }>
  ) => {
    const create = containerCreateFlow(
      trigger.pipe(map((t) => pick(t, ["sub", "accessToken"])))
    )

    const polling = containerPollingFlow(
      zip(create.success, trigger).pipe(
        map(([c, t]) => ({
          ...pick(c, ["containerID"]),
          ...pick(t, ["accessToken"]),
        }))
      )
    )

    const host = hostServiceFlow(
      zip([trigger, create.success]).pipe(
        map(([t, _c]) => pick(t, ["sub", "accessToken", "configToken"]))
      )
    )

    return {
      success: createTrigger("containerFlowSuccess", polling.success),
      failure: createTrigger(
        "containerFlowFailure",
        merge(create.failure, polling.failure, host.failure)
      ),
    }
  }
)

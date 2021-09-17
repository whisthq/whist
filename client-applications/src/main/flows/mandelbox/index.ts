import { merge, Observable, zip } from "rxjs"
import { map } from "rxjs/operators"
import mandelboxCreateFlow from "@app/main/flows/mandelbox/create"
import hostSpinUpFlow from "@app/main/flows/mandelbox/host"
import { flow } from "@app/utils/flows"

export default flow(
  "mandelboxFlow",
  (
    trigger: Observable<{
      accessToken: string
      configToken: string
      region?: any
    }>
  ) => {
    const create = mandelboxCreateFlow(
      trigger.pipe(
        map((t) => ({
          accessToken: t.accessToken,
          region: t.region,
        }))
      )
    )

    const host = hostSpinUpFlow(
      zip([trigger, create.success]).pipe(
        map(([t, c]) => ({
          ip: c.ip,
          config_encryption_token: t.configToken,
          jwt_access_token: t.accessToken,
          mandelbox_id: c.mandelboxID,
        }))
      )
    )

    return {
      success: host.success,
      failure: merge(create.failure, host.failure),
    }
  }
)

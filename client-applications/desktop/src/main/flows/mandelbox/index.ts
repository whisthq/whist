import { merge, Observable, zip } from "rxjs"
import { map } from "rxjs/operators"
import { pick } from "lodash"
import mandelboxCreateFlow from "@app/main/flows/mandelbox/create"
import hostSpinUpFlow from "@app/main/flows/mandelbox/host"
import { flow } from "@app/utils/flows"
import { AWSRegion } from "@app/@types/aws"

export default flow(
  "mandelboxFlow",
  (
    trigger: Observable<{
      sub: string
      accessToken: string
      configToken: string
      region?: AWSRegion
    }>
  ) => {
    const create = mandelboxCreateFlow(
      trigger.pipe(map((t) => pick(t, ["sub", "accessToken", "region"])))
    )

    const host = hostSpinUpFlow(
      zip([trigger, create.success]).pipe(
        map(([t, c]) => ({
          ip: c.ip,
          dpi: 1000000,
          user_id: t.sub,
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

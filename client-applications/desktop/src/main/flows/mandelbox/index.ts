import { merge, Observable, zip } from "rxjs"
import { map } from "rxjs/operators"
import mandelboxCreateFlow from "@app/main/flows/mandelbox/create"
import hostSpinUpFlow from "@app/main/flows/mandelbox/host"
import { flow } from "@app/utils/flows"
import { AWSRegion } from "@app/@types/aws"
import { getDPI } from "@app/utils/mandelbox"

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
    const dpi = getDPI()

    const create = mandelboxCreateFlow(
      trigger.pipe(
        map((t) => ({
          sub: t.sub,
          accessToken: t.accessToken,
          dpi: dpi,
          region: t.region,
        }))
      )
    )

    const host = hostSpinUpFlow(
      zip([trigger, create.success]).pipe(
        map(([t, c]) => ({
          ip: c.ip,
          dpi: dpi,
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

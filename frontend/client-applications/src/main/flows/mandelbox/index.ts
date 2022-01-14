import { merge, Observable, zip } from "rxjs"
import { map } from "rxjs/operators"
import mandelboxCreateFlow from "@app/main/flows/mandelbox/create"
import hostSpinUpFlow from "@app/main/flows/mandelbox/host"
import { flow } from "@app/utils/flows"
import { persistGet } from "@app/utils/persist"
import { RESTORE_LAST_SESSION } from "@app/constants/store"
import { AWSRegion } from "@app/@types/aws"
import { appEnvironment } from "config/build"
import { WhistEnvironments } from "config/constants"

export default flow(
  "mandelboxFlow",
  (
    trigger: Observable<{
      accessToken: string
      configToken: string
      isNewConfigToken: boolean
      cookies: string
      bookmarks: string
      extensions: string
      userEmail: string
      regions: Array<{ region: AWSRegion; pingTime: number }>
      darkMode: boolean
      timezone: string
      dpi: number
      keyRepeat: number | undefined
      initialKeyRepeat: number | undefined
    }>
  ) => {
    const create = mandelboxCreateFlow(
      trigger.pipe(
        map((t) => ({
          accessToken: t.accessToken,
          userEmail: t.userEmail,
          regions: t.regions.map((r) => r.region),
        }))
      )
    )

    const host = hostSpinUpFlow(
      zip([trigger, create.success]).pipe(
        map(([t, c]) => ({
          ip: c.ip,
          configToken: t.configToken,
          accessToken: t.accessToken,
          isNewConfigToken: t.isNewConfigToken,
          mandelboxID: c.mandelboxID,
          cookies: t.cookies,
          bookmarks: t.bookmarks,
          extensions: t.extensions,
          jsonData: JSON.stringify({
            dark_mode: t.darkMode,
            desired_timezonetime: t.timezone,
            client_dpi: t.dpi,
            restore_last_session: persistGet(RESTORE_LAST_SESSION) ?? true,
            initial_key_repeat: t.initialKeyRepeat,
            key_repeat: t.keyRepeat,
            ...(t.isNewConfigToken && {
              initial_url: "https://whist.typeform.com/to/Oi21wwbg",
            }),
            ...(appEnvironment === WhistEnvironments.LOCAL && {
              local_client: true,
            }),
          }), // Data to send through the JSON transport
        }))
      )
    )

    return {
      success: host.success,
      failure: merge(create.failure, host.failure),
    }
  }
)

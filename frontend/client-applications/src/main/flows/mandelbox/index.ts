import { screen } from "electron"

import { merge, Observable, of } from "rxjs"
import {
  map,
  withLatestFrom,
  timeout,
  catchError,
  takeUntil,
  switchMap,
} from "rxjs/operators"
import mandelboxCreateFlow from "@app/main/flows/mandelbox/create"
import hostSpinUpFlow from "@app/main/flows/mandelbox/host"
import { flow } from "@app/main/utils/flows"
import { persistGet } from "@app/main/utils/persist"
import { RESTORE_LAST_SESSION } from "@app/constants/store"
import { AWSRegion } from "@app/@types/aws"
import { appEnvironment } from "config/build"
import { WhistEnvironments } from "config/constants"
import { getUserAgent } from "@app/main/utils/userAgent"

export default flow(
  "mandelboxFlow",
  (
    trigger: Observable<{
      accessToken: string
      configToken: string
      isNewConfigToken: boolean
      importedData:
        | { cookies: string; bookmarks: string; extensions: string }
        | undefined
      userEmail: string
      regions: Array<{ region: AWSRegion; pingTime: number }>
      darkMode: boolean
      timezone: string
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
      create.success.pipe(withLatestFrom(trigger)).pipe(
        map(([c, t]) => ({
          ip: c.ip,
          configToken: t.configToken,
          accessToken: t.accessToken,
          isNewConfigToken: t.isNewConfigToken,
          mandelboxID: c.mandelboxID,
          importedData: t.importedData,
          jsonData: JSON.stringify({
            dark_mode: t.darkMode,
            desired_timezonetime: t.timezone,
            user_agent: getUserAgent(),
            client_dpi: screen.getPrimaryDisplay()?.scaleFactor * 96,
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
      timeout: trigger.pipe(
        switchMap(() =>
          merge(host.success, create.failure, host.failure).pipe(
            timeout(20000), // If nothing is emitted for 20s, we assume a timeout so that an error can be shown
            catchError(() => of(undefined)),
            takeUntil(merge(host.success, create.failure, host.failure))
          )
        )
      ),
    }
  }
)

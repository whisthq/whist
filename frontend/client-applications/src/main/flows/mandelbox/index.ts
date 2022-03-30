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

export default flow(
  "mandelboxFlow",
  (
    trigger: Observable<{
      accessToken: string
      configToken: string
      isNewConfigToken: boolean
      importedData:
        | {
            cookies: string
            bookmarks: string
            extensions: string
            preferences: string
          }
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
            desired_timezone: t.timezone,
            client_dpi: screen.getPrimaryDisplay()?.scaleFactor * 96,
            restore_last_session: persistGet(RESTORE_LAST_SESSION) ?? true,
            kiosk_mode: false, // We only want the server-side Chromium to be in Kiosk mode if the client is running within a Chromium browser, not within Electron
            initial_key_repeat: t.initialKeyRepeat,
            key_repeat: t.keyRepeat,
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
          // We use switchmap so that the timer starts only when the trigger fires vs. when the flow is run
          merge(host.success, create.failure, host.failure).pipe(
            timeout(25000), // If nothing is emitted for 25s, we assume a timeout so that an error can be shown
            catchError(() => of(undefined)),
            takeUntil(merge(host.success, create.failure, host.failure))
          )
        )
      ),
    }
  }
)

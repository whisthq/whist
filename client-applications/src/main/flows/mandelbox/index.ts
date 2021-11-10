import { merge, Observable, zip, from, race } from "rxjs"
import {
  map,
  switchMap,
  share,
  withLatestFrom,
  delay,
  filter,
} from "rxjs/operators"
import mandelboxCreateFlow from "@app/main/flows/mandelbox/create"
import hostSpinUpFlow from "@app/main/flows/mandelbox/host"
import { flow } from "@app/utils/flows"
import { nativeTheme } from "electron"
import { persistGet } from "@app/utils/persist"
import { getJSONDecryptedCookies, InstalledBrowser } from "@app/utils/importer"
import { RESTORE_LAST_SESSION } from "@app/constants/store"
import { getInitialKeyRepeat, getKeyRepeat } from "@app/utils/keyRepeat"

export default flow(
  "mandelboxFlow",
  (
    trigger: Observable<{
      accessToken: string
      configToken: string
      isNewConfigToken: boolean
      importCookiesFrom: string | undefined
    }>
  ) => {
    const create = mandelboxCreateFlow(
      trigger.pipe(
        map((t) => ({
          accessToken: t.accessToken,
        }))
      )
    )

    // Retrieve keyboard repeat rates to send them to the mandelbox
    const initialKeyRepeat = getInitialKeyRepeat()
    const keyRepeat = getKeyRepeat()

    const decrypted = trigger.pipe(
      switchMap((t) =>
        from(getJSONDecryptedCookies(t.importCookiesFrom as InstalledBrowser))
      ),
      share() // If you don't share, this observable will fire many times (once for each subscriber of the flow)
    )

    const host = hostSpinUpFlow(
      zip([trigger, create.success, decrypted]).pipe(
        map(([t, c, d]) => ({
          ip: c.ip,
          configToken: t.configToken,
          accessToken: t.accessToken,
          cookies: d,
          mandelboxID: c.mandelboxID,
          jsonData: JSON.stringify({
            dark_mode: nativeTheme.shouldUseDarkColors,
            desired_timezone: Intl.DateTimeFormat().resolvedOptions().timeZone,
            restore_last_session: persistGet(RESTORE_LAST_SESSION) ?? true,
            ...(initialKeyRepeat !== "" &&
              !isNaN(parseInt(initialKeyRepeat)) && {
                initial_key_repeat: parseInt(initialKeyRepeat),
              }),
            ...(keyRepeat !== "" &&
              !isNaN(parseInt(keyRepeat)) && {
                key_repeat: parseInt(keyRepeat),
              }),
          }), // Data to send through the JSON transport
        }))
      )
    )

    // The server protocol takes time to start up when we upload cookies, so this is a temporary
    // workaround to delay trying to connect if we send over cookies
    const hostWithDelay = host.success.pipe(
      withLatestFrom(trigger),
      filter(([, t]) => t.importCookiesFrom !== undefined),
      delay(5000),
      map(([h]) => h)
    )

    const hostWithoutDelay = host.success.pipe(
      withLatestFrom(trigger),
      filter(([, t]) => t.importCookiesFrom === undefined),
      map(([h]) => h)
    )

    return {
      success: race(hostWithDelay, hostWithoutDelay),
      failure: merge(create.failure, host.failure),
    }
  }
)

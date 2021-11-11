import { merge, Observable, zip, from, race } from "rxjs"
import { map, switchMap, share } from "rxjs/operators"
import mandelboxCreateFlow from "@app/main/flows/mandelbox/create"
import hostSpinUpFlow from "@app/main/flows/mandelbox/host"
import { flow } from "@app/utils/flows"
import { nativeTheme } from "electron"
import { persistGet } from "@app/utils/persist"
import { getDecryptedCookies, InstalledBrowser } from "@app/utils/importer"
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
        from(getDecryptedCookies(t.importCookiesFrom as InstalledBrowser))
      ),
      share() // If you don't share, this observable will fire many times (once for each subscriber of the flow)
    )

    const host = hostSpinUpFlow(
      zip([trigger, create.success, decrypted]).pipe(
        map(([t, c, d]) => ({
          ip: c.ip,
          configToken: t.configToken,
          accessToken: t.accessToken,
          cookies: JSON.stringify(d),
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

    return {
      success: host.success,
      failure: merge(create.failure, host.failure),
    }
  }
)

import { merge, Observable, zip } from "rxjs"
import { map } from "rxjs/operators"
import mandelboxCreateFlow from "@app/main/flows/mandelbox/create"
import hostSpinUpFlow from "@app/main/flows/mandelbox/host"
import { flow } from "@app/utils/flows"
import { nativeTheme } from "electron"
import { persistGet } from "@app/utils/persist"
import { RESTORE_LAST_SESSION } from "@app/constants/store"
import { getInitialKeyRepeat, getKeyRepeat } from "@app/utils/keyRepeat"

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
    }>
  ) => {
    const create = mandelboxCreateFlow(
      trigger.pipe(
        map((t) => ({
          accessToken: t.accessToken,
          userEmail: t.userEmail,
        }))
      )
    )

    // Retrieve keyboard repeat rates to send them to the mandelbox
    const initialKeyRepeat = getInitialKeyRepeat()
    const keyRepeat = getKeyRepeat()

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
            ...(t.isNewConfigToken && {
              initial_url: "https://whist.typeform.com/to/Oi21wwbg",
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

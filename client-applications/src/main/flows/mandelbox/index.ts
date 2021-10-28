import { merge, Observable, zip } from "rxjs"
import { map } from "rxjs/operators"
import mandelboxCreateFlow from "@app/main/flows/mandelbox/create"
import hostSpinUpFlow from "@app/main/flows/mandelbox/host"
import { flow } from "@app/utils/flows"
import { nativeTheme } from "electron"
import { isNumber } from "lodash"
import { execCommand } from "./../../../../scripts/execCommand"
import { persistGet } from "@app/utils/persist"

export default flow(
  "mandelboxFlow",
  (
    trigger: Observable<{
      accessToken: string
      configToken: string
    }>
  ) => {
    const create = mandelboxCreateFlow(
      trigger.pipe(
        map((t) => ({
          accessToken: t.accessToken,
        }))
      )
    )

    // Retrieve macOS keyboard repeat rates to send them to the mandelbox
    const initialKeyRepeat = execCommand(
      "defaults read NSGlobalDomain InitialKeyRepeat",
      ".",
      {},
      "pipe"
    )
    const keyRepeat = execCommand(
      "defaults read NSGlobalDomain KeyRepeat",
      ".",
      {},
      "pipe"
    )

    const host = hostSpinUpFlow(
      zip([trigger, create.success]).pipe(
        map(([t, c]) => ({
          ip: c.ip,
          config_encryption_token: t.configToken,
          jwt_access_token: t.accessToken,
          json_data: JSON.stringify({
            dark_mode: nativeTheme.shouldUseDarkColors,
            desired_tz: Intl.DateTimeFormat().resolvedOptions().timeZone,
            restore_last_session: persistGet(
              "restoreLastChromeSession",
              "data"
            ),
            initial_key_repeat: isNumber(initialKeyRepeat)
              ? initialKeyRepeat
              : 68, // this fails if the user hasn't modified the default value, which is 68
            key_repeat: isNumber(keyRepeat) ? keyRepeat : 6, // this fails if the user hasn't modified the default value, which is 6
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

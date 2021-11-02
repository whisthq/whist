import { merge, Observable, zip } from "rxjs"
import { map } from "rxjs/operators"
import mandelboxCreateFlow from "@app/main/flows/mandelbox/create"
import hostSpinUpFlow from "@app/main/flows/mandelbox/host"
import { flow } from "@app/utils/flows"
import { nativeTheme } from "electron"
import { execCommandByOS } from "@app/utils/execCommand"
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

    // Retrieve keyboard repeat rates to send them to the mandelbox
    let initialKeyRepeat = execCommandByOS(
      "defaults read NSGlobalDomain InitialKeyRepeat",
      /* eslint-disable no-template-curly-in-string */
      "key_repeat_str=($(xset -q | grep 'auto repeat delay')) && echo ${key_repeat_str[3]}",
      null,
      ".",
      {},
      "pipe"
    )
    if (typeof initialKeyRepeat !== "number") {
      initialKeyRepeat = -1
    }

    let keyRepeat = execCommandByOS(
      "defaults read NSGlobalDomain KeyRepeat",
      /* eslint-disable no-template-curly-in-string */
      "key_repeat_str=($(xset -q | grep 'auto repeat delay')) && echo ${key_repeat_str[6]}",
      null,
      ".",
      {},
      "pipe"
    )
    if (typeof keyRepeat !== "number") {
      keyRepeat = -1
    }

    const host = hostSpinUpFlow(
      zip([trigger, create.success]).pipe(
        map(([t, c]) => ({
          ip: c.ip,
          config_encryption_token: t.configToken,
          jwt_access_token: t.accessToken,
          mandelbox_id: c.mandelboxID,
          json_data: JSON.stringify({
            dark_mode: nativeTheme.shouldUseDarkColors,
            desired_tz: Intl.DateTimeFormat().resolvedOptions().timeZone,
            restore_last_session:
              persistGet("RestoreLastBrowserSession", "data") ?? "true",
            ...(initialKeyRepeat >= 0 && {
              initial_key_repeat: initialKeyRepeat,
            }),
            ...(keyRepeat >= 0 && { key_repeat: keyRepeat }),
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

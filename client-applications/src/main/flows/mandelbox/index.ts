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
    const initialKeyRepeatRaw = execCommandByOS(
      "defaults read NSGlobalDomain InitialKeyRepeat",
      /* eslint-disable no-template-curly-in-string */
      "xset -q | grep 'auto repeat delay'",
      "",
      ".",
      {},
      "pipe"
    )

    let initialKeyRepeat =
      initialKeyRepeatRaw !== null ? initialKeyRepeatRaw.toString() : ""
    // Remove trailing '\n'
    initialKeyRepeat.replace(/\n$/, "")
    // Extract value from bash output
    if (process.platform === "linux" && initialKeyRepeat !== "") {
      const startIndex =
        initialKeyRepeat.indexOf("auto repeat delay:") +
        "auto repeat delay:".length +
        2
      const endIndex = initialKeyRepeat.indexOf("repeat rate:") - 4
      initialKeyRepeat = initialKeyRepeat.substring(startIndex, endIndex)
    }

    const keyRepeatRaw = execCommandByOS(
      "defaults read NSGlobalDomain KeyRepeat",
      /* eslint-disable no-template-curly-in-string */
      "xset -q | grep 'auto repeat delay'",
      "",
      ".",
      {},
      "pipe"
    )

    let keyRepeat = keyRepeatRaw !== null ? keyRepeatRaw.toString() : ""

    // Remove trailing '\n'
    keyRepeat.replace(/\n$/, "")
    // Extract value from bash output
    if (process.platform === "linux" && keyRepeat !== "") {
      const startIndex =
        keyRepeat.indexOf("repeat rate:") + "repeat rate:".length + 2
      const endIndex = keyRepeat.length
      keyRepeat = keyRepeat.substring(startIndex, endIndex)
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
            desired_timezone: Intl.DateTimeFormat().resolvedOptions().timeZone,
            restore_last_session: persistGet(
              "RestoreLastBrowserSession",
              "data"
            ),
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

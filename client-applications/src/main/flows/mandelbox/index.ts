import { merge, Observable, zip, from } from "rxjs"
import { map, switchMap, share } from "rxjs/operators"
import mandelboxCreateFlow from "@app/main/flows/mandelbox/create"
import hostSpinUpFlow from "@app/main/flows/mandelbox/host"
import { flow } from "@app/utils/flows"
import { nativeTheme } from "electron"
import { execCommandByOS } from "@app/utils/execCommand"
import { persistGet, persistSet } from "@app/utils/persist"
import { getDecryptedCookies, InstalledBrowser } from "@app/utils/importer"
import { RESTORE_LAST_SESSION } from "@app/constants/store"

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
    } else if (process.platform === "darwin" && initialKeyRepeat !== "") {
      // Convert the key repetition delay from Mac scale (shortest=15, longest=120) to Linux scale (shortest=115, longest=2000)
      const initialKeyRepeatMinValMac: number = 15.0
      const initialKeyRepeatMaxValMac: number = 120.0
      const initialKeyRepeatRangeMac: number =
        initialKeyRepeatMaxValMac - initialKeyRepeatMinValMac
      const initialKeyRepeatMinValLinux: number = 115.0
      const initialKeyRepeatMaxValLinux: number = 2000.0
      const initialKeyRepeatRangeLinux: number =
        initialKeyRepeatMaxValLinux - initialKeyRepeatMinValLinux
      const initialKeyRepeatFloat: number =
        ((parseInt(initialKeyRepeat) - initialKeyRepeatMinValMac) /
          initialKeyRepeatRangeMac) *
          initialKeyRepeatRangeLinux +
        initialKeyRepeatMinValLinux
      initialKeyRepeat = initialKeyRepeatFloat.toFixed()
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
    } else if (process.platform === "darwin" && keyRepeat !== "") {
      // Convert the key repetition delay from Mac scale (slowest=120, fastest=2) to Linux scale (slowest=9, fastest=1000). NB: the units on Mac and Linux are multiplicative inverse.
      const keyRepeatMinValMac: number = 2.0
      const keyRepeatMaxValMac: number = 120.0
      const keyRepeatRangeMac: number = keyRepeatMaxValMac - keyRepeatMinValMac
      const keyRepeatMinValLinux: number = 9.0
      const keyRepeatMaxValLinux: number = 1000.0
      const keyRepeatRangeLinux: number =
        keyRepeatMaxValLinux - keyRepeatMinValLinux
      const keyRepeatFloat: number =
        (1.0 - (parseInt(keyRepeat) - keyRepeatMinValMac) / keyRepeatRangeMac) *
          keyRepeatRangeLinux +
        keyRepeatMinValLinux
      keyRepeat = keyRepeatFloat.toFixed()
    }

    if (persistGet(RESTORE_LAST_SESSION) === undefined) {
      persistSet(RESTORE_LAST_SESSION, true)
    }

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
          mandelboxID: c.mandelboxID,
          cookies: d,
          jsonData: JSON.stringify({
            dark_mode: nativeTheme.shouldUseDarkColors,
            desired_timezone: Intl.DateTimeFormat().resolvedOptions().timeZone,
            restore_last_session: persistGet(RESTORE_LAST_SESSION),
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

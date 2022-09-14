import { from, of } from "rxjs"
import {
  switchMap,
  map,
  filter,
  share,
  timeout,
  catchError,
} from "rxjs/operators"

import { getStorage } from "@app/worker/utils/storage"
import { hostSpinUp, hostSpinUpSuccess } from "@app/worker/utils/host"
import {
  timeZone,
  darkMode,
  userAgent,
  platform,
  keyboardRepeatRate,
  keyboardRepeatInitialDelay,
} from "@app/worker/utils/jsonTransport"
import { mandelboxSuccess } from "@app/worker/events/mandelbox"

import { MandelboxInfo } from "@app/@types/payload"
import { Storage } from "@app/constants/storage"
import { AsyncReturnType } from "@app/@types/api"
import { generateRandomConfigToken } from "@app/@core-ts/auth"

const jsonTransport = async () => {
  const t = timeZone()
  const u = userAgent()

  const [d, p, r, i] = await Promise.all([
    darkMode(),
    platform(),
    keyboardRepeatRate(),
    keyboardRepeatInitialDelay(),
  ])

  return {
    dark_mode: d,
    desired_timezone: t,
    user_agent: u,
    client_os: p,
    client_dpi: window.devicePixelRatio * 96,
    key_repeat: r,
    initial_key_repeat: i,
  }
}

const hostInfo = mandelboxSuccess.pipe(
  switchMap((mandelbox: MandelboxInfo) =>
    from(
      Promise.all([
        getStorage(Storage.AUTH_INFO),
        getStorage(Storage.CONFIG_TOKEN_INFO),
        new Promise((resolve) => resolve(mandelbox)),
        jsonTransport(),
      ])
    )
  ),
  switchMap(([auth, _configTokenInfo, mandelbox, jsonData]) =>
    from(
      hostSpinUp({
        ip: mandelbox.mandelboxIP ?? "",
        config_encryption_token: generateRandomConfigToken(),
        is_new_config_encryption_token: true,
        jwt_access_token: auth.accessToken ?? "",
        mandelbox_id: mandelbox.mandelboxID,
        json_data: JSON.stringify({
          kiosk_mode: true,
          restore_last_session: false,
          load_extension: true,
          ...jsonData,
        }),
        importedData: undefined,
      })
    ).pipe(
      timeout(10000), // If nothing is emitted for 10s, we assume a timeout so that an error can be shown
      catchError(() => of({}))
    )
  ),
  share()
)

const hostSuccess = hostInfo.pipe(
  filter((response: AsyncReturnType<typeof hostSpinUp>) =>
    hostSpinUpSuccess(response)
  ),
  map((response: AsyncReturnType<typeof hostSpinUp>) => ({
    mandelboxSecret: response.json?.result?.aes_key,
    mandelboxPorts: {
      port_32261: response.json?.result?.port_32261,
      port_32262: response.json?.result?.port_32262,
      port_32263: response.json?.result?.port_32263,
      port_32273: response.json?.result?.port_32273,
    },
  })),
  share()
)

const hostError = hostInfo.pipe(
  filter(
    (response: AsyncReturnType<typeof hostSpinUp>) =>
      !hostSpinUpSuccess(response)
  ),
  share()
)

export { hostSuccess, hostError }

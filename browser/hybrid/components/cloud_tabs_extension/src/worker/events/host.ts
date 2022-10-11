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
import {
  hostSpinUp,
  hostSpinUpSuccess,
  HostSpinUpResponse,
} from "@app/worker/utils/host"
import {
  timeZone,
  darkMode,
  userAgent,
  platform,
  keyboardRepeatRate,
  keyboardRepeatInitialDelay,
  userLocale,
  browserLanguages,
  systemLanguages,
  location,
} from "@app/worker/utils/jsonTransport"
import { mandelboxSuccess } from "@app/worker/events/mandelbox"

import { AuthInfo, MandelboxInfo } from "@app/@types/payload"
import { Storage } from "@app/constants/storage"

const jsonTransport = async () => {
  const t = timeZone()
  const u = userAgent()

  const [d, p, r, i, systemLangs] =
    await Promise.all([
      darkMode(),
      platform(),
      keyboardRepeatRate(),
      keyboardRepeatInitialDelay(),
      systemLanguages(),
    ])

  return {
    dark_mode: d,
    desired_timezone: t,
    user_agent: u,
    client_os: p,
    client_dpi: window.devicePixelRatio * 96,
    key_repeat: r,
    initial_key_repeat: i,
    system_languages: systemLangs,
  }
}

const hostInfo = mandelboxSuccess.pipe(
  switchMap((mandelbox: MandelboxInfo) =>
    from(
      Promise.all([
        getStorage<AuthInfo>(Storage.AUTH_INFO),
        // TODO: This is silly and probably not needed.
        new Promise<MandelboxInfo>((resolve) => resolve(mandelbox)),
        jsonTransport(),
      ])
    )
  ),
  switchMap(([auth, mandelbox, jsonData]) =>
    from(
      hostSpinUp({
        ip: mandelbox.mandelboxIP ?? "",
        jwt_access_token: auth.accessToken ?? "",
        mandelbox_id: mandelbox.mandelboxID,
        json_data: JSON.stringify({
          kiosk_mode: true,
          restore_last_session: false,
          load_extension: true,
          ...jsonData,
        }),
      })
    ).pipe(
      timeout(10000), // If nothing is emitted for 10s, we assume a timeout so that an error can be shown
      catchError(() => of({} as HostSpinUpResponse))
    )
  ),
  share()
)

const hostSuccess = hostInfo.pipe(
  filter((response: HostSpinUpResponse) => hostSpinUpSuccess(response)),
  map((response: HostSpinUpResponse) => ({
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
  filter((response: HostSpinUpResponse) => !hostSpinUpSuccess(response)),
  share()
)

export { hostSuccess, hostError }

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
import { mandelboxSuccess } from "@app/worker/events/mandelbox"

import { AuthInfo, MandelboxInfo } from "@app/@types/payload"
import { Storage } from "@app/constants/storage"

const hostInfo = mandelboxSuccess.pipe(
  switchMap((mandelbox: MandelboxInfo) =>
    from(
      Promise.all([
        getStorage<AuthInfo>(Storage.AUTH_INFO),
        // TODO: This is silly and probably not needed.
        new Promise<MandelboxInfo>((resolve) => resolve(mandelbox)),
      ])
    )
  ),
  switchMap(([auth, mandelbox]) =>
    from(
      hostSpinUp({
        ip: mandelbox.mandelboxIP ?? "",
        jwt_access_token: auth.accessToken ?? "",
        mandelbox_id: mandelbox.mandelboxID,
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

import {
  hostSpinUp,
  hostSpinUpValid,
  hostSpinUpError,
  HostSpinUpResponse,
} from "@app/utils/host"
import { zip, from } from "rxjs"
import { map, switchMap } from "rxjs/operators"
import { flow, fork } from "@app/utils/flows"

import { accessToken, configToken } from "@fractal/core-ts"
import { Cookie } from "@app/utils/importer"

export default flow<
  {
    ip: string
    jsonData: string
    cookies: string
    mandelboxID: string
  } & accessToken &
    configToken
>("hostSpinUpFlow", (trigger) => {
  const spin = fork(
    trigger.pipe(
      switchMap((args) =>
        from(
          hostSpinUp({
            ip: args.ip,
            config_encryption_token: args.configToken,
            jwt_access_token: args.accessToken,
            json_data: args.jsonData,
            mandelbox_id: args.mandelboxID,
            cookies: args.cookies,
          })
        )
      )
    ),
    {
      success: (result: HostSpinUpResponse) => hostSpinUpValid(result),
      failure: (result: HostSpinUpResponse) => hostSpinUpError(result),
    }
  )

  return {
    success: zip([trigger, spin.success]).pipe(
      map(([t, s]) => ({
        mandelboxIP: t.ip,
        mandelboxSecret: s.json?.result?.aes_key,
        mandelboxPorts: {
          port_32262: s.json?.result?.port_32262,
          port_32263: s.json?.result?.port_32263,
          port_32273: s.json?.result?.port_32273,
        },
      }))
    ),
    failure: spin.failure,
  }
})

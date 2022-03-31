import {
  hostSpinUp,
  hostSpinUpValid,
  hostSpinUpError,
  HostSpinUpResponse,
} from "@app/main/utils/host"
import { zip, from } from "rxjs"
import { map, switchMap } from "rxjs/operators"
import { flow, fork } from "@app/main/utils/flows"

import { accessToken, configToken } from "@whist/core-ts"

export default flow<
  {
    ip: string
    jsonData: string
    mandelboxID: string
    isNewConfigToken: boolean
    importedData:
      | {
          cookies: string
          bookmarks: string
          extensions: string
          preferences: string
        }
      | undefined
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
            is_new_config_encryption_token: args.isNewConfigToken,
            importedData: args.importedData,
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

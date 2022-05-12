import { zip } from "rxjs"

import { createEvent, fromEvent } from "@app/utils/events"

import { getStorage } from "@app/utils/storage"
import { generateRandomConfigToken } from "@app/@core-ts/auth"

import { WorkerMessageType } from "@app/constants/messaging"
import { Storage } from "@app/constants/storage"
import { hostSpinUp, hostSpinUpSuccess } from "@app/utils/host"
import { AuthInfo } from "@app/@types/payload"
import { MandelboxInfo } from "@app/@types/payload"

const getConfigToken = async () => {
  const cachedConfigToken =
    (await getStorage(Storage.CONFIG_TOKEN)) ?? undefined
  return {
    configToken: (cachedConfigToken ?? generateRandomConfigToken()) as string,
    isNew: cachedConfigToken === undefined,
  }
}

const initHostSpinUp = () => {
  zip(
    fromEvent(WorkerMessageType.AUTH_SUCCESS),
    fromEvent(WorkerMessageType.MANDELBOX_ASSIGN_SUCCESS)
  ).subscribe(async ([auth, mandelbox]: [AuthInfo, MandelboxInfo]) => {
    console.log("Host spin up", auth, mandelbox)
    const { configToken, isNew } = await getConfigToken()

    const response = await hostSpinUp({
      ip: mandelbox.ip,
      config_encryption_token: configToken,
      is_new_config_encryption_token: isNew,
      jwt_access_token: auth.accessToken,
      mandelbox_id: mandelbox.mandelboxID,
      json_data: "",
      importedData: undefined,
    })

    console.log("host response was", response)

    if (!hostSpinUpSuccess(response)) return

    createEvent(WorkerMessageType.HOST_SPINUP_SUCCESS, {
      secret: response.json?.result?.aes_key,
      ports: {
        port_32262: response.json?.result?.port_32262,
        port_32263: response.json?.result?.port_32263,
        port_32273: response.json?.result?.port_32273,
      },
    })
  })
}

export { initHostSpinUp }

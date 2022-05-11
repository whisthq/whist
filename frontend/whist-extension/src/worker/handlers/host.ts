import { zip } from "rxjs"

import { ipcMessage } from "@app/worker/utils/messaging"

import { getStorage } from "@app/worker/utils/storage"
import { generateRandomConfigToken } from "@app/worker/@core-ts/auth"

import { ContentScriptMessageType } from "@app/constants/messaging"
import { Storage } from "@app/constants/storage"
import { hostSpinUp, hostSpinUpSuccess } from "@app/worker/utils/host"
import { AuthInfo } from "@app/@types/auth"
import { MandelboxInfo } from "@app/@types/mandelbox"

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
    ipcMessage(ContentScriptMessageType.AUTH_SUCCESS),
    ipcMessage(ContentScriptMessageType.MANDELBOX_ASSIGN_SUCCESS)
  ).subscribe(async ([auth, mandelbox]: [AuthInfo, MandelboxInfo]) => {
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

    if (!hostSpinUpSuccess(response)) return

    chrome.runtime.sendMessage({
      type: ContentScriptMessageType.HOST_SPINUP_SUCCESS,
      value: {
        secret: response.json?.result?.aes_key,
        ports: {
          port_32262: response.json?.result?.port_32262,
          port_32263: response.json?.result?.port_32263,
          port_32273: response.json?.result?.port_32273,
        },
      },
    })
  })
}

export { initHostSpinUp }

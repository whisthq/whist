import { shell } from "electron"

import { fromTrigger } from "@app/main/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"

fromTrigger(WhistTrigger.openExternalURL).subscribe(
  (payload: { url: string }) => {
    shell.openExternal(payload.url).catch((err) => console.error(err))
  }
)

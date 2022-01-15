import { fromTrigger } from "@app/utils/flows"
import { createGlobalShortcut } from "@app/utils/shortcuts"

import { WhistTrigger } from "@app/constants/triggers"
import {
  getWindowByHash,
  createOmnibar,
  destroyOmnibar,
} from "@app/utils/windows"
import { WindowHashOmnibar } from "@app/constants/windows"

fromTrigger(WhistTrigger.mandelboxFlowSuccess).subscribe(() => {
  createGlobalShortcut("CommandorControl+J", () => {
    const win = getWindowByHash(WindowHashOmnibar)

    win === undefined ? createOmnibar() : destroyOmnibar()
  })
})

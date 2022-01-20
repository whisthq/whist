import { fromTrigger } from "@app/main/utils/flows"
import { createGlobalShortcut } from "@app/main/utils/shortcuts"

import { WhistTrigger } from "@app/constants/triggers"
import {
  getWindowByHash,
  createOmnibar,
  destroyOmnibar,
} from "@app/main/utils/renderer"
import { WindowHashOmnibar } from "@app/constants/windows"

fromTrigger(WhistTrigger.mandelboxFlowSuccess).subscribe(() => {
  createGlobalShortcut("CommandorControl+J", () => {
    const win = getWindowByHash(WindowHashOmnibar)

    win === undefined ? createOmnibar() : destroyOmnibar()
  })
})

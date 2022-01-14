import { fromTrigger } from "@app/utils/flows"
import { createGlobalShortcut } from "@app/utils/shortcuts"

import { WhistTrigger } from "@app/constants/triggers"
import { getWindowByHash, showOmnibar, hideOmnibar } from "@app/utils/windows"
import { WindowHashOmnibar } from "@app/constants/windows"

fromTrigger(WhistTrigger.mandelboxFlowSuccess).subscribe(() => {
  createGlobalShortcut("Option+W", () => {
    const win = getWindowByHash(WindowHashOmnibar)

    win?.isVisible() ?? false ? hideOmnibar() : showOmnibar()
  })
})

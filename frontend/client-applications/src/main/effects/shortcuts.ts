import { fromTrigger } from "@app/utils/flows"
import { createGlobalShortcut } from "@app/utils/shortcuts"

import { WhistTrigger } from "@app/constants/triggers"
import { getWindowByHash } from "@app/utils/windows"
import { WindowHashOmnibar } from "@app/constants/windows"

fromTrigger(WhistTrigger.mandelboxFlowSuccess).subscribe(() => {
  createGlobalShortcut("Option+A", () => {
    const win = getWindowByHash(WindowHashOmnibar)

    if (win?.isVisible() ?? false) {
      win?.hide()
    } else {
      win?.show()
      win?.focus()
    }
  })
})

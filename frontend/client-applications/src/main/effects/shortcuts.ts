/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains effects that deal with keyboard shortcuts
 */

import { fromTrigger } from "@app/main/utils/flows"
import { createGlobalShortcut } from "@app/main/utils/shortcuts"

import { WhistTrigger } from "@app/constants/triggers"
import {
  showElectronWindow,
  hideElectronWindow,
  isElectronWindowVisible,
} from "@app/main/utils/windows"
import { WindowHashOmnibar } from "@app/constants/windows"

fromTrigger(WhistTrigger.mandelboxFlowSuccess).subscribe(() => {
  createGlobalShortcut("CommandorControl+J", () => {
    const visible = isElectronWindowVisible(WindowHashOmnibar)
    visible
      ? hideElectronWindow(WindowHashOmnibar)
      : showElectronWindow(WindowHashOmnibar)
  })
})

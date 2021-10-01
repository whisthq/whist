import AutoLaunch from "auto-launch"
import path from "path"
import { app } from "electron"

import { fromTrigger } from "@app/utils/flows"
import config from "@app/config/environment"
import { persist, persistGet } from "@app/utils/persist"
import { protocolStreamKill } from "@app/utils/protocol"
import { createErrorWindow, relaunch } from "@app/utils/windows"
import { WindowHashSleep } from "@app/utils/constants"
import { fromSignal } from "@app/utils/observables"

const fractalAutoLaunch = new AutoLaunch({
  name: config.title,
  path: path.join(app.getPath("exe"), "../../../"),
})

/* eslint-disable @typescript-eslint/no-floating-promises */
fromTrigger("appReady").subscribe(() => {
  fractalAutoLaunch
    .isEnabled()
    .then((enabled: boolean) => {
      if (!enabled && persistGet("autoLaunch", "data") === undefined) {
        fractalAutoLaunch.enable()
        persist("autoLaunch", "true", "data")
      }
    })
    .catch((err: any) => console.error(err))
})

fromTrigger("trayAutolaunchAction").subscribe(() => {
  const autolaunch = (persistGet("autoLaunch", "data") ?? "true") === "true"
  persist("autoLaunch", (!autolaunch).toString(), "data")

  autolaunch ? fractalAutoLaunch.disable() : fractalAutoLaunch.enable()
})

fromSignal(fromTrigger("powerSuspend"), fromTrigger("appReady")).subscribe(
  () => {
    createErrorWindow(WindowHashSleep)
    protocolStreamKill()
  }
)

fromTrigger("powerResume").subscribe(() => {
  relaunch()
})

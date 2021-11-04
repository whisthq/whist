import { fromTrigger } from "@app/utils/flows"
import { protocolStreamKill } from "@app/utils/protocol"
import { createErrorWindow, relaunch } from "@app/utils/windows"
import { WindowHashSleep } from "@app/constants/windows"
import { fromSignal } from "@app/utils/observables"
import { WhistTrigger } from "@app/constants/triggers"

fromSignal(
  fromTrigger(WhistTrigger.powerSuspend),
  fromTrigger(WhistTrigger.appReady)
).subscribe(() => {
  createErrorWindow(WindowHashSleep)
  protocolStreamKill()
})

fromTrigger(WhistTrigger.powerResume).subscribe(() => {
  relaunch()
})

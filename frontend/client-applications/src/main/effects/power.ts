import { fromTrigger } from "@app/main/utils/flows"
import { protocolStreamKill } from "@app/main/utils/protocol"
import { createErrorWindow, relaunch } from "@app/main/utils/windows"
import { WindowHashSleep } from "@app/constants/windows"
import { fromSignal } from "@app/main/utils/observables"
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

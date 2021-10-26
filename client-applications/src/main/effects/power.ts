import { fromTrigger } from "@app/utils/flows"
import { protocolStreamKill } from "@app/utils/protocol"
import { createErrorWindow, relaunch } from "@app/utils/windows"
import { WindowHashSleep } from "@app/constants/windows"
import { fromSignal } from "@app/utils/observables"

fromSignal(fromTrigger("powerSuspend"), fromTrigger("appReady")).subscribe(
  () => {
    createErrorWindow(WindowHashSleep)
    protocolStreamKill()
  }
)

fromTrigger("powerResume").subscribe(() => {
  relaunch()
})

import { fromTrigger } from "@app/utils/flows"
import { logBase, LogLevel } from "@app/utils/logging"

fromTrigger("monitoringStatus").subscribe((clientStatus: string) => {
  logBase("monitoring.success", { status: clientStatus }, LogLevel.DEBUG).catch(
    (err) => console.log(err)
  )
})

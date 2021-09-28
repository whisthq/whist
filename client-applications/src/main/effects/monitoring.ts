import { logBase } from "@app/utils/logging"
import { interval } from "rxjs"

const LOG_INTERVAL_IN_MINUTES = 5

interval(LOG_INTERVAL_IN_MINUTES * 60 * 1000).subscribe(() => {
  logBase("heartbeat", {})
})

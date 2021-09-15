import { interval, map } from "rxjs"

import { createTrigger } from "@app/utils/flows"
import TRIGGER from "@app/utils/triggers"

const minutes = 1

// Trigger for monitoring the status of the application every number of minutes
createTrigger(
  TRIGGER.monitoringStatus,
  interval((minutes > 0 ? minutes : 10) * 60 * 1000).pipe(map(() => "active"))
) // Emit the status of the client

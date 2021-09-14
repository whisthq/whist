import { flow } from "@app/utils/flows"
import { interval, map } from "rxjs"

const minutes = 10

// Flow for monitoring the status of the application every number of minutes
export default flow("monitoringFlow", () => {
  return {
    success: interval(minutes * 60 * 1000).pipe(map(() => "active")),
  }
})

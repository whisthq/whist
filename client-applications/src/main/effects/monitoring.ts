import { logBase } from "@app/utils/logging"
import * as Sentry from "@sentry/electron"
import { interval, race, of } from "rxjs"
import { delay } from "rxjs/operators"

import config from "@app/config/environment"
import { appEnvironment, FractalEnvironments } from "../../../config/configs"
import { HEARTBEAT_INTERVAL_IN_MINUTES, SENTRY_DSN } from "@app/constants/app"
import { fromTrigger } from "@app/utils/flows"
import {
  networkAnalyze,
  TEST_DURATION_SECONDS,
} from "@app/utils/networkAnalysis"

// Initialize and report Sentry errors in prod
if (appEnvironment === FractalEnvironments.PRODUCTION) {
  Sentry.init({
    dsn: SENTRY_DSN,
    environment: config.deployEnv,
    release: `client-applications@${config.keys.COMMIT_SHA as string}`,
  })
}
const MAX_ACCEPTABLE_JITTER_MS = 20
const MIN_ACCEPTABLE_DOWNLOAD_MBPS = 20

networkAnalyze()

interval(HEARTBEAT_INTERVAL_IN_MINUTES * 60 * 1000).subscribe(() => {
  logBase("heartbeat", {})
})

race(
  fromTrigger("networkAnalysis"),
  of(undefined).pipe(delay(TEST_DURATION_SECONDS + 3000))
).subscribe(
  (results: { jitter: number; downloadSpeed: number } | undefined) => {
    if (results === undefined) return

    if (
      results.jitter > MAX_ACCEPTABLE_JITTER_MS ||
      results.downloadSpeed < MIN_ACCEPTABLE_DOWNLOAD_MBPS
    )
      console.log("network unstable")
  }
)

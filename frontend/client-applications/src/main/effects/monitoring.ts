import { logBase } from "@app/utils/logging"
import * as Sentry from "@sentry/electron"
import { interval } from "rxjs"

import config from "@app/config/environment"
import { appEnvironment, WhistEnvironments } from "../../../config/configs"
import { HEARTBEAT_INTERVAL_IN_MINUTES, SENTRY_DSN } from "@app/constants/app"
import { networkAnalyze } from "@app/utils/networkAnalysis"
import { fromTrigger } from "@app/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"

// Initialize and report Sentry errors in prod
if (appEnvironment === WhistEnvironments.PRODUCTION) {
  Sentry.init({
    dsn: SENTRY_DSN,
    environment: config.deployEnv,
    release: `client-applications@${config.keys.COMMIT_SHA as string}`,
  })
}

fromTrigger(WhistTrigger.startNetworkAnalysis).subscribe(() => {
  networkAnalyze()
})

interval(HEARTBEAT_INTERVAL_IN_MINUTES).subscribe(() => {
  logBase("heartbeat", {})
})

import { logBase } from "@app/main/utils/logging"
import * as Sentry from "@sentry/electron"
import { interval } from "rxjs"

import config from "@app/config/environment"
import { appEnvironment, WhistEnvironments } from "../../../config/configs"
import { HEARTBEAT_INTERVAL_IN_MS, SENTRY_DSN } from "@app/constants/app"
import { networkAnalyze } from "@app/main/utils/networkAnalysis"
import { fromTrigger } from "@app/main/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"
import { withAppActivated } from "@app/main/utils/observables"

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

withAppActivated(interval(HEARTBEAT_INTERVAL_IN_MS)).subscribe(() => {
  logBase("heartbeat", {})
})

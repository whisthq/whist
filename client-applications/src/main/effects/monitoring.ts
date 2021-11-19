import { logBase } from "@app/utils/logging"
import { interval } from "rxjs"
import * as Sentry from "@sentry/electron"

import config from "@app/config/environment"
import { appEnvironment, FractalEnvironments } from "../../../config/configs"
import { HEARTBEAT_INTERVAL_IN_MINUTES, SENTRY_DSN } from "@app/constants/app"

// Initialize and report Sentry errors in prod
if (appEnvironment === FractalEnvironments.PRODUCTION) {
  Sentry.init({
    dsn: SENTRY_DSN,
    environment: config.deployEnv,
    release: `client-applications@${config.keys.COMMIT_SHA as string}`,
  })
}

interval(HEARTBEAT_INTERVAL_IN_MINUTES * 60 * 1000).subscribe(() => {
  logBase("heartbeat", {})
})

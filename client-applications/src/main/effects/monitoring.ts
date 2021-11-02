import { logBase } from "@app/utils/logging"
import { interval } from "rxjs"
import * as Sentry from "@sentry/electron"

import config from "@app/config/environment"
import { appEnvironment, FractalEnvironments } from "../../../config/configs"
import { fromTrigger } from "@app/utils/flows"
import {
  sessionID,
  logzProtocolClientURL,
  logzProtocolServerURL,
  LOG_INTERVAL_IN_MINUTES,
  SENTRY_DSN,
} from "@app/utils/constants"
import { persistGet } from "@app/utils/persist"

// Initialize and report Sentry errors in all packaged environments
if (appEnvironment !== FractalEnvironments.LOCAL) {
  Sentry.init({
    dsn: SENTRY_DSN,
    environment: config.sentryEnv,
    release: `client-applications@${config.keys.COMMIT_SHA as string}`,
  })

  Sentry.setContext("Logz URLs", {
    protocolClientLogs: logzProtocolClientURL(),
    protocolServerLogs: logzProtocolServerURL(),
  })

  fromTrigger("protocolError").subscribe(() => {
    Sentry.captureMessage(
      `Protocol error detected for user ${
        (persistGet("userEmail") ?? "") as string
      } with session ID ${sessionID.toString()}. Investigate immediately.`,
      Sentry.Severity.Error
    )
  })
}

interval(LOG_INTERVAL_IN_MINUTES * 60 * 1000).subscribe(() => {
  logBase("heartbeat", {})
})

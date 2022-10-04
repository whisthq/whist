import { fromEvent, of, merge } from "rxjs"

import { serverCookiesSynced } from "@app/worker/events/cookies"
import { languagesInitialized } from "@app/worker/events/language"

// All events that reflect tasks that need to have been completed before we
//     begin creating cloud tabs
const initSettingsSent = merge(
  serverCookiesSynced,
  languagesInitialized,
)

export { initSettingsSent }
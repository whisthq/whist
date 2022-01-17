import { map, startWith } from "rxjs/operators"
import { Observable, of } from "rxjs"
import { nativeTheme } from "electron"

import { fromTrigger } from "@app/main/utils/flows"
import { persistGet } from "@app/main/utils/persist"
import {
  CACHED_ACCESS_TOKEN,
  CACHED_REFRESH_TOKEN,
  CACHED_USER_EMAIL,
  CACHED_CONFIG_TOKEN,
} from "@app/constants/store"
import { WhistTrigger } from "@app/constants/triggers"
import { withAppReady } from "@app/utils/observables"
import { getInitialKeyRepeat, getKeyRepeat } from "@app/utils/keyRepeat"

// Auth state
const accessToken = fromTrigger(WhistTrigger.authFlowSuccess).pipe(
  map((x) => x.accessToken ?? ""),
  startWith(persistGet(CACHED_ACCESS_TOKEN) ?? "")
) as Observable<string>

const refreshToken = fromTrigger(WhistTrigger.authFlowSuccess).pipe(
  map((x) => x.refreshToken ?? ""),
  startWith(persistGet(CACHED_REFRESH_TOKEN) ?? "")
) as Observable<string>

const userEmail = fromTrigger(WhistTrigger.authFlowSuccess).pipe(
  map((x) => x.userEmail ?? ""),
  startWith(persistGet(CACHED_USER_EMAIL) ?? "")
) as Observable<string>

const configToken = fromTrigger(WhistTrigger.authFlowSuccess).pipe(
  map((x) => x.configToken ?? ""),
  startWith(persistGet(CACHED_CONFIG_TOKEN) ?? "")
) as Observable<string>

const isNewConfigToken = of(persistGet(CACHED_CONFIG_TOKEN) ?? "").pipe(
  map((x) => x === "")
)

// JSON transport state e.g. system settings
const darkMode = withAppReady(of(nativeTheme.shouldUseDarkColors))
const timezone = of(Intl.DateTimeFormat().resolvedOptions().timeZone)
const keyRepeat = of(getKeyRepeat())
const initialKeyRepeat = of(getInitialKeyRepeat())

export {
  accessToken,
  refreshToken,
  userEmail,
  configToken,
  isNewConfigToken,
  darkMode,
  timezone,
  keyRepeat,
  initialKeyRepeat,
}

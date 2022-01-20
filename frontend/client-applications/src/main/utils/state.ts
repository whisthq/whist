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
import {
  DEFAULT_LINUX_USER_AGENT,
  DEFAULT_MAC_USER_AGENT,
  DEFAULT_WINDOWS_USER_AGENT,
} from "@app/constants/app"
import { WhistTrigger } from "@app/constants/triggers"
import { withAppReady } from "@app/main/utils/observables"
import { getInitialKeyRepeat, getKeyRepeat } from "@app/main/utils/keyRepeat"

const sleep = of(process.argv[process.argv.length - 1] === "--sleep")

const accessToken = fromTrigger(WhistTrigger.storeDidChange).pipe(
  map(() => (persistGet(CACHED_ACCESS_TOKEN) as string) ?? ""),
  startWith(persistGet(CACHED_ACCESS_TOKEN) ?? "")
) as Observable<string>

const refreshToken = fromTrigger(WhistTrigger.storeDidChange).pipe(
  map(() => (persistGet(CACHED_REFRESH_TOKEN) as string) ?? ""),
  startWith(persistGet(CACHED_REFRESH_TOKEN) ?? "")
) as Observable<string>

const userEmail = fromTrigger(WhistTrigger.storeDidChange).pipe(
  map(() => (persistGet(CACHED_USER_EMAIL) as string) ?? ""),
  startWith(persistGet(CACHED_USER_EMAIL) ?? "")
) as Observable<string>

const configToken = fromTrigger(WhistTrigger.storeDidChange).pipe(
  map(() => (persistGet(CACHED_CONFIG_TOKEN) as string) ?? ""),
  startWith(persistGet(CACHED_CONFIG_TOKEN) ?? "")
) as Observable<string>

const isNewConfigToken = of(persistGet(CACHED_CONFIG_TOKEN) ?? "").pipe(
  map((x) => x === "")
)

const getUserAgent = () => {
  if (process.platform === "linux") {
    return DEFAULT_LINUX_USER_AGENT
  } else if (process.platform === "darwin") {
    return DEFAULT_MAC_USER_AGENT
  } else {
    // Windows!
    return DEFAULT_WINDOWS_USER_AGENT
  }
}

// JSON transport state e.g. system settings
const darkMode = withAppReady(of(nativeTheme.shouldUseDarkColors))
const timezone = of(Intl.DateTimeFormat().resolvedOptions().timeZone)
const keyRepeat = of(getKeyRepeat())
const initialKeyRepeat = of(getInitialKeyRepeat())

const userAgent = of(getUserAgent())

export {
  sleep,
  accessToken,
  refreshToken,
  userEmail,
  configToken,
  isNewConfigToken,
  darkMode,
  timezone,
  keyRepeat,
  initialKeyRepeat,
  userAgent,
}

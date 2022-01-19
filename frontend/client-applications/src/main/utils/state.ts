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
    return "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/97.0.4692.71 Safari/537.36"
  } else if (process.platform === "darwin") {
    return "Mozilla/5.0 (Macintosh; Intel Mac OS X 12_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/97.0.4692.71 Safari/537.36"
  } else {
    return "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/97.0.4692.71 Safari/537.36"
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

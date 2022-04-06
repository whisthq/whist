import { Observable, of, from } from "rxjs"
import { map, startWith, timeout, catchError } from "rxjs/operators"
import { nativeTheme } from "electron"

import { fromTrigger } from "@app/main/utils/flows"
import { persistGet } from "@app/main/utils/persist"
import {
  CACHED_ACCESS_TOKEN,
  CACHED_REFRESH_TOKEN,
  CACHED_USER_EMAIL,
  CACHED_CONFIG_TOKEN,
  GEOLOCATION,
} from "@app/constants/store"
import { WhistTrigger } from "@app/constants/triggers"
import { withAppActivated } from "@app/main/utils/observables"
import { getInitialKeyRepeat, getKeyRepeat } from "@app/main/utils/keyRepeat"
import { getUserLocale } from "@app/main/utils/userLocale"
import { getInstalledBrowsers } from "@app/main/utils/importer"
import { getGeolocation } from "./location"

const sleep = of(process.argv.includes("--sleep"))

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
const geolocation = from(getGeolocation()).pipe(
  timeout(1500),
  catchError(() => {
    console.warn("Geolocation timed out")
    return of(persistGet(GEOLOCATION))
  })
)

// JSON transport state e.g. system settings
const darkMode = withAppActivated(of(nativeTheme.shouldUseDarkColors))
const timezone = of(Intl.DateTimeFormat().resolvedOptions().timeZone)
const keyRepeat = of(getKeyRepeat())
const initialKeyRepeat = of(getInitialKeyRepeat())
const userLocale = of(getUserLocale())

// Keep track of what browsers are installed
const browsers = of(getInstalledBrowsers())

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
  browsers,
  geolocation,
  userLocale,
}

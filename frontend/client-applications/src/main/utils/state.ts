import { Observable, of, from } from "rxjs"
import { map, startWith, switchMap } from "rxjs/operators"
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
import { getInstalledBrowsers } from "@app/main/utils/importer"
import { getGeolocation } from "@app/main/utils/location"

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
const geoLocation = fromTrigger(WhistTrigger.storeDidChange).pipe(
  map(() => (persistGet(GEOLOCATION) as string) ?? ""),
  startWith(persistGet(GEOLOCATION) ?? "")
) as Observable<object>

// JSON transport state e.g. system settings
const darkMode = withAppActivated(of(nativeTheme.shouldUseDarkColors))
const timezone = of(Intl.DateTimeFormat().resolvedOptions().timeZone)
const keyRepeat = of(getKeyRepeat())
const initialKeyRepeat = of(getInitialKeyRepeat())

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
  geoLocation,
}

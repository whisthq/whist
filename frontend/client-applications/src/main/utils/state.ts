import { map, startWith } from "rxjs/operators"
import { Observable, of } from "rxjs"

import { fromTrigger } from "@app/main/utils/flows"
import { persistGet } from "@app/main/utils/persist"
import {
  CACHED_ACCESS_TOKEN,
  CACHED_REFRESH_TOKEN,
  CACHED_USER_EMAIL,
  CACHED_CONFIG_TOKEN,
} from "@app/constants/store"
import { WhistTrigger } from "@app/constants/triggers"

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

export { accessToken, refreshToken, userEmail, configToken, isNewConfigToken }

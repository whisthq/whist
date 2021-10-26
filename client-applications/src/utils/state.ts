import { map, startWith } from "rxjs/operators"
import { Observable } from "rxjs"

import { fromTrigger } from "@app/utils/flows"
import { persistGet } from "@app/utils/persist"
import {
  CACHED_ACCESS_TOKEN,
  CACHED_REFRESH_TOKEN,
  CACHED_USER_EMAIL,
  CACHED_CONFIG_TOKEN,
} from "@app/constants/store"

const accessToken = fromTrigger("authFlowSuccess").pipe(
  map((x) => x.accessToken ?? ""),
  startWith(persistGet(CACHED_ACCESS_TOKEN) ?? "")
) as Observable<string>

const refreshToken = fromTrigger("authFlowSuccess").pipe(
  map((x) => x.refreshToken ?? ""),
  startWith(persistGet(CACHED_REFRESH_TOKEN) ?? "")
) as Observable<string>

const userEmail = fromTrigger("authFlowSuccess").pipe(
  map((x) => x.userEmail ?? ""),
  startWith(persistGet(CACHED_USER_EMAIL) ?? "")
) as Observable<string>

const configToken = fromTrigger("authFlowSuccess").pipe(
  map((x) => x.configToken ?? ""),
  startWith(persistGet(CACHED_CONFIG_TOKEN) ?? "")
) as Observable<string>

export { accessToken, refreshToken, userEmail, configToken }

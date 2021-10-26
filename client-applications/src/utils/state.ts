import { map, startWith } from "rxjs/operators"
import { Observable } from "rxjs"

import { fromTrigger } from "@app/utils/flows"
import { persistedAuth } from "@app/utils/persist"

const accessToken = fromTrigger("authFlowSuccess").pipe(
  map((x) => x.accessToken ?? ""),
  startWith(persistedAuth?.accessToken ?? "")
) as Observable<string>

const refreshToken = fromTrigger("authFlowSuccess").pipe(
  map((x) => x.refreshToken ?? ""),
  startWith(persistedAuth?.refreshToken ?? "")
) as Observable<string>

const userEmail = fromTrigger("authFlowSuccess").pipe(
  map((x) => x.userEmail ?? ""),
  startWith(persistedAuth?.userEmail ?? "")
) as Observable<string>

const configToken = fromTrigger("authFlowSuccess").pipe(
  map((x) => x.configToken ?? ""),
  startWith(persistedAuth?.accessToken ?? "")
) as Observable<string>

export { accessToken, refreshToken, userEmail, configToken }

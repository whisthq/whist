import { map, startWith } from "rxjs/operators"

import { fromTrigger } from "@app/utils/flows"
import { persistedAuth } from "@app/utils/persist"

const accessToken = fromTrigger("authFlowSuccess").pipe(
  map((x) => x.accessToken),
  startWith(persistedAuth?.accessToken)
)

const refreshToken = fromTrigger("authFlowSuccess").pipe(
  map((x) => x.refreshToken),
  startWith(persistedAuth?.refreshToken)
)

const userEmail = fromTrigger("authFlowSuccess").pipe(
  map((x) => x.userEmail),
  startWith(persistedAuth?.userEmail)
)

const configToken = fromTrigger("authFlowSuccess").pipe(
  map((x) => x.configToken),
  startWith(persistedAuth?.accessToken)
)

export { accessToken, refreshToken, userEmail, configToken }

import { from, of } from "rxjs"
import {
  switchMap,
  map,
  filter,
  share,
  timeout,
  catchError,
} from "rxjs/operators"

import {
  getPolicies,
  getPoliciesSuccess,
  GetPoliciesResponse,
} from "@app/worker/utils/policy"
import { authSuccess } from "@app/worker/events/auth"

import { AuthInfo } from "@app/@types/payload"

const policyRetrieval = authSuccess.pipe(
  switchMap((authInfo: AuthInfo) =>
    from(getPolicies(authInfo.accessToken ?? ""))
  ),
  timeout(10000), // If nothing is emitted for 10s, we assume a timeout so that an error can be shown
  catchError(() =>
    of({
      json: {
        error: "Client Timeout",
      },
    } as GetPoliciesResponse)
  )
)

const policyRetrievalSuccess = policyRetrieval.pipe(
  filter((response: GetPoliciesResponse) => getPoliciesSuccess(response)),
  map((response: GetPoliciesResponse) => ({
    policy: response.json?.policy,
  })),
  share()
)

const policyRetrievalError = policyRetrieval.pipe(
  filter((response: GetPoliciesResponse) => !getPoliciesSuccess(response)),
  share()
)

export { policyRetrievalSuccess, policyRetrievalError }

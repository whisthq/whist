import { policyRetrievalSuccess } from "@app/worker/events/policy"
import { applyPolicies } from "@app/worker/utils/policy"

policyRetrievalSuccess.subscribe(({ policy }) => {
  applyPolicies(policy)
})

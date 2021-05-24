import { merge } from "rxjs"

import { fromTrigger } from "@app/utils/flows"
import { persist } from "@app/utils/persist"

// On a successful auth, store the email and tokens in Electron store
// so the user is remembered
merge(
  fromTrigger("loginFlowSuccess"),
  fromTrigger("signupFlowSuccess")
).subscribe(
  (args: { email: string; accessToken: string; configToken: string }) => {
    persist("email", args.email)
    persist("accessToken", args.accessToken)
    persist("configToken", args.configToken)
  }
)

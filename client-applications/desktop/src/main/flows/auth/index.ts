import { merge } from "rxjs"

import loginFlow from "@app/main/flows/auth/flows/login"
import signupFlow from "@app/main/flows/auth/flows/signup"
import persistFlow from "@app/main/flows/auth/flows/persist"
import { loginAction, signupAction } from "@app/main/events/actions"
import { flow } from "@app/utils/flows"

export default flow("authFlow", (name, trigger) => {
  const persisted = persistFlow(name, trigger)
  const login = loginFlow(name, loginAction)
  const signup = signupFlow(name, signupAction)

//   persisted.failure.subscribe(() => createAuthWindow((win: any) => win.show()))

  const success = merge(login.success, signup.success, persisted.success)
  const failure = merge(login.failure, signup.failure)

//   merge(login.success, signup.success).subscribe((creds) =>
//     persist(omit(creds, ["password"]))
//   )

  return { success, failure }
})

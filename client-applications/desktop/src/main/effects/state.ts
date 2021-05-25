import { merge } from "rxjs"
import { pluck } from "rxjs/operators"
import { fromTrigger, StateChannel } from "@app/utils/flows"

// Add email to state
merge(
  fromTrigger("persisted"),
  fromTrigger("loginFlowSuccess"),
  fromTrigger("signupFlowSuccess")
)
  .pipe(pluck("email"))
  .subscribe((value) => StateChannel.next({ email: value }))

// Add closing flag to state
merge(
  fromTrigger("protocolCloseFlowSuccess"),
  fromTrigger("protocolCloseFlowSuccess")
).subscribe(() => {
  StateChannel.next({ closing: true })
})

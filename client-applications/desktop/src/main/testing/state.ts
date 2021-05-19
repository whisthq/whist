import { Observable } from "rxjs"
import { mapTo, tap, delay } from "rxjs/operators"

type MockSchema = {
  [key: string]: (trigger: Observable<any>) => {
    [key: string]: Observable<any>
  }
}

const loginError: MockSchema = {
  loginFlow: (trigger) => ({
    failure: trigger.pipe(
      delay(2000),
      mapTo({ status: 400, json: { access_token: "" } }),
      tap(() => console.log("MOCKED FAILURE"))
    ),
  }),
}

export default { loginError }

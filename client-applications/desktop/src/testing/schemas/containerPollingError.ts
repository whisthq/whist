import { mapTo, tap, delay } from "rxjs/operators"
import { MockSchema } from "@app/@types/schema"

const containerPollingError: MockSchema = {
  containerPollingInner: (trigger) => ({
    failure: trigger.pipe(
      delay(2000),
      mapTo({ response: { json: { state: "" } } }),
      tap(() => console.log("MOCKED CONTAINER POLLING FAILURE"))
    ),
  }),
}

export default containerPollingError

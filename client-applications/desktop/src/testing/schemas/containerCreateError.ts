import { mapTo, tap, delay } from "rxjs/operators"
import { MockSchema } from "@app/@types/schema"

const containerCreateError: MockSchema = {
  containerFlow: (trigger) => ({
    failure: trigger.pipe(
      delay(2000),
      mapTo({ req: { json: { ID: "" } } }),
      tap(() => console.log("MOCKED CONTAINER CREATE FAILURE"))
    ),
  }),
}

export default containerCreateError

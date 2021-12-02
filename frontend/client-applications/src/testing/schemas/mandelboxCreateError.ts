import { mapTo, tap, delay } from "rxjs/operators"
import { MockSchema } from "@app/@types/schema"

const mandelboxCreateError: MockSchema = {
  mandelboxFlow: (trigger) => ({
    failure: trigger.pipe(
      delay(2000),
      mapTo({ json: { ID: "", error: "COMMIT_HASH_MISMATCH" }, status: 503 }),
      tap(() => console.log("MOCKED MANDELBOX CREATE FAILURE"))
    ),
  }),
}

export default mandelboxCreateError

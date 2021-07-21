import { mapTo, tap, delay } from "rxjs/operators"
import { MockSchema } from "@app/@types/schema"

const mandelboxPollingError: MockSchema = {
    mandelboxPollingInner: (trigger) => ({
        failure: trigger.pipe(
            delay(2000),
            mapTo({ response: { json: { state: "" } } }),
            tap(() => console.log("MOCKED MANDELBOX POLLING FAILURE"))
        ),
    }),
}

export default mandelboxPollingError

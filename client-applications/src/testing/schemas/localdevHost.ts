/*
 * Mocks the mandelboxCreateFlow to hardcode connecting to a specific
 * host service. This is useful when modifying the API between the
 * client app and the host service.
 *
 * Usage:
 *   TESTING_LOCALDEV_HOST_IP=[ip address] yarn test:manual localdevHost
 *
 * Note:
 *   If you want to actually work with the mandelbox, make sure to build
 *   it beforehand. If built using `build_local_mandelbox.sh`, make sure
 *   to call `run_local_mandelbox.sh` before attempting to run the above,
 *   as this will create a new mandelbox with the correct protocol build
 *   instead of an outdated one.
 */
import { mapTo, tap } from "rxjs/operators"
import { MockSchema } from "@app/@types/schema"
import crypto from "crypto";

const localdevHost: MockSchema = {
  mandelboxCreateFlow: (trigger) => ({
    success: trigger.pipe(
      tap(() => {
        if (process.env?.TESTING_LOCALDEV_HOST_IP == null) {
          const message =
            "Host IP address not set! Set the `TESTING_LOCALDEV_HOST_IP` environment variable to use this schema."
          throw new Error(message)
        }
      }),
      mapTo({
        mandelboxID: crypto.randomBytes(8).toString("hex"),
        ip: process.env.TESTING_LOCALDEV_HOST_IP,
      }),
      tap(() => console.log("HARDCODED HOST SERVICE IP"))
    ),
  }),
}

export default localdevHost

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

const localdevHost: MockSchema = {
  mandelboxCreateFlow: (trigger) => ({
    success: trigger.pipe(
      mapTo({
        mandelboxID: "0123456789abcdef0123456789abcd",
        ip: process.env.TESTING_LOCALDEV_HOST_IP,
      }),
      tap(() => console.log("HARDCODED HOST SERVICE IP"))
    ),
  }),
}

export default localdevHost

import { mapTo, tap, delay } from "rxjs/operators"
import { MockSchema } from "@app/@types/schema"

/*

export const hostConfigFailure = hostConfigProcess.pipe(
  filter((res: { status: number }) => hostServiceConfigError(res))
)

export const protocolLaunchFailure = merge(
  hostConfigFailure,
  containerPollingFailure
)

Protocol launch failure is tirggered as a result of a merge between 
host config failure and contaiener polling failure. The output of this
failure needs to be edited to reflect this functionality. 
*/

const protocolLaunchError: MockSchema = {
  protocolLaunchFlow: (trigger) => ({
    failure: trigger.pipe(tap(() => console.log("MOCKED FAILURE"))),
  }),
}

export default protocolLaunchError

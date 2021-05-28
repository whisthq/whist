import { tap } from "rxjs/operators"
import { MockSchema } from "@app/@types/schema"

/*

export const hostConfigFailure = hostConfigProcess.pipe(
  filter((res: { status: number }) => hostServiceConfigError(res))
)

export const protocolLaunchFailure = merge(
  hostConfigFailure,
  mandelboxPollingFailure
)

Protocol launch failure is tirggered as a result of a merge between 
host config failure and mandelbox polling failure. The output of this
failure needs to be edited to reflect this functionality. 
*/

const protocolLaunchError: MockSchema = {
  protocolLaunchFlow: (trigger) => ({
    failure: trigger.pipe(tap(() => console.log("MOCKED FAILURE"))),
  }),
}

export default protocolLaunchError

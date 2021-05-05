import { Observable, merge } from "rxjs"
import { withLatestFrom } from "rxjs/operators"
import ua from "universal-analytics"

import { FractalCIEnvironment } from "../../config/environment"
import { userEmail } from "@app/main/observables/user"
import env from "@app/utils/env"

const GOOGLE_ANALYTICS_ID = "UA-180615646-1"
const visitor = ua(GOOGLE_ANALYTICS_ID)

export const sendGAEvent = (action: string, ...obs: Array<Observable<any>>) => {
//   if (env.PACKAGED_ENV === FractalCIEnvironment.PRODUCTION)
    merge(...obs)
      .pipe(withLatestFrom(userEmail))
      .subscribe(([, email]: [any, string]) => {
        const params = {
          ec: "Electron",
          ea: action,
          el: email,
        }

        visitor.event(params).send()
      })
}

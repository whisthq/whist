// This file is home to observables that manage mandelbox creation.
// Their responsibilities are to listen for state that will trigger protocol
// launches. These observables then communicate with the webserver and
// poll the state of the mandelboxs while they spin up.
//
// These observables are subscribed by protocol launching observables, which
// react to success mandelbox creation emissions from here.

import { from } from "rxjs"
import { map, switchMap } from "rxjs/operators"

import { mandelboxCreate, mandelboxCreateSuccess } from "@app/utils/mandelbox"
import { fork, flow } from "@app/utils/flows"
import { AWSRegion } from "@app/@types/aws"

export default flow<{
    subClaim: string
    accessToken: string
    region?: AWSRegion
}>("mandelboxCreateFlow", (trigger) => {
    const create = fork(
        trigger.pipe(
            switchMap(({ subClaim, accessToken, region }) =>
                from(mandelboxCreate(subClaim, accessToken, region))
            )
        ),
        {
            success: (req) => mandelboxCreateSuccess(req),
            failure: (req) => !mandelboxCreateSuccess(req),
        }
    )

    return {
        success: create.success.pipe(
            map((response: { json: { ID: string } }) => ({
                mandelboxID: response.json.ID,
            }))
        ),
        failure: create.failure,
    }
})

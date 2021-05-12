// This file is home to observables that manage container creation.
// Their responsibilities are to listen for state that will trigger protocol
// launches. These observables then communicate with the webserver and
// poll the state of the containers while they spin up.
//
// These observables are subscribed by protocol launching observables, which
// react to success container creation emissions from here.

import { from } from "rxjs"
import { map, switchMap } from "rxjs/operators"

import {
  containerCreate,
  containerCreateSuccess,
} from "@app/main/flows/container/flows/create/utils"
import { fork, flow } from "@app/utils/flows"

export default flow(
  "containerCreateFlow",
  (trigger) => {
    const create = fork(
      trigger.pipe(
        switchMap(({ email, accessToken }) =>
          from(containerCreate(email, accessToken))
        )
      ),
      {
        success: (req) => containerCreateSuccess(req),
        failure: (req) => !containerCreateSuccess(req),
      }
    )

    return {
      success: create.success.pipe(
        map((response: {json: {ID: string}}) => ({ containerID: response.json.ID }))
      ),
      failure: create.failure,
    }
  }
)
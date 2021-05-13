import { Observable } from "rxjs"
import { filter, map, pluck } from "rxjs/operators"

import { eventIPC } from "@app/main/triggers/ipc"
import { ActionType, RendererAction, Action } from "@app/@types/actions"
import { trigger } from "@app/main/utils/flows"

const filterType = (observable: Observable<Action>, type: ActionType) =>
  observable.pipe(
    filter((x: Action) => x !== undefined && x.type === type),
    map((x: Action) => x.payload)
  )

// Action triggers
trigger(
  "loginAction",
  filterType(eventIPC.pipe(pluck("action")), RendererAction.LOGIN)
)
trigger(
  "signupAction",
  filterType(eventIPC.pipe(pluck("action")), RendererAction.SIGNUP)
)
trigger(
  "relaunchAction",
  filterType(eventIPC.pipe(pluck("action")), RendererAction.RELAUNCH)
)

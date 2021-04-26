import { filter } from "rxjs/operators"

import { fromEventIPC } from "@app/main/events/ipc";
import { fromEventTray } from "@app/main/events/tray"
import { SubscriptionMap } from "@app/utils/observables"

export enum UserAction {
  LOGIN = "loginRequest",
  SIGNUP = "signupRequest",
  SIGNOUT = "signoutRequest",
  QUIT = "quitRequest",
}

export const actionMap: SubscriptionMap = {
    loginRequest: fromEventIPC(UserAction.LOGIN),
    signupRequest: fromEventIPC(UserAction.SIGNUP),
    signoutRequest: fromEventTray(UserAction.SIGNOUT),
    quitRequest: fromEventTray(UserAction.QUIT)
} 

export const fromAction = (action: UserAction) => actionMap[action]
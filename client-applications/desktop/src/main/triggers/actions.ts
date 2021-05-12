import { Observable } from "rxjs"
import { filter, map, share } from "rxjs/operators"

// import { fromEventIPC } from "@app/main/triggers/ipc"
import { eventTray } from "@app/main/triggers/tray"
import {
  ActionType,
  MainAction,
  RendererAction,
  Action,
} from "@app/@types/actions"
import { trigger } from "@app/main/utils/flows"

const action = (type: ActionType): Observable<any> => {
  /*
        Description:
            An action is defined as any event that the user generates by interacting with the client
            app, usually by clicking on buttons or typing. This observable will emit every time
            the desired action type (e.g. login request, signout request) is detected.

        Usage:
            const loginAction = action(RendererAction.LOGIN).pipe(
                tap(x => console.log("x"))
            )

            // Output: {email: test@test.com, password: Test123}

        Arguments:
            type (ActionType): The action type, see @types/actions for all available types
    */

  // Helper function which takes an Action-emitting observable and emits only Actions that match
  // the type passed in above
  const filterType = (observable: Observable<Action>) =>
    observable.pipe(
      filter((x: Action) => x !== undefined && x.type === type),
      map((x: Action) => x.payload)
    )

  /* eslint-disable @typescript-eslint/strict-boolean-expressions */
  // if ((<any>Object).values(RendererAction).includes(type))
  //   return filterType(fromEventIPC("action")).pipe(share())

  return filterType(eventTray).pipe(share())
}

// Action triggers
export const loginAction = trigger("loginAction", action(RendererAction.LOGIN))
export const signupAction = trigger("signupAction", action(RendererAction.SIGNUP))
export const rendererAction = trigger("rendererAction", action(RendererAction.RELAUNCH))
export const signoutAction = trigger("signoutAction", action(MainAction.SIGNOUT))
export const quitAction = trigger("quitAction", action(MainAction.QUIT))

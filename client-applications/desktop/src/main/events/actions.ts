import { fromEventIPC } from "@app/main/events/ipc"
import { eventTray } from "@app/main/events/tray"
import { Observable } from "rxjs"
import { filter, map, share } from "rxjs/operators"
import {
  ActionType,
  MainAction,
  RendererAction,
  Action,
} from "@app/@types/actions"

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
  if ((<any>Object).values(RendererAction).includes(type))
    return filterType(fromEventIPC("action"))

  return filterType(eventTray)
}

// Should all actions be "hot"?
export const loginAction = action(RendererAction.LOGIN)
export const signupAction = action(RendererAction.SIGNUP)
export const rendererAction = action(RendererAction.RELAUNCH)

export const signoutAction = action(MainAction.SIGNOUT).pipe(share())
export const quitAction = action(MainAction.QUIT).pipe(share())

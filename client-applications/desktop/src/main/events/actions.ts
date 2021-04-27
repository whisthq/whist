import { fromEventIPC } from "@app/main/events/ipc"
import { eventTray } from "@app/main/events/tray"
import { Observable } from "rxjs"
import { filter, map, share } from "rxjs/operators"
import {
  ActionType,
  RendererAction,
  TrayAction,
  Action,
} from "@app/@types/actions"

export const action = (type: ActionType): Observable<any> => {
  /*
        Description:
            An action is defined as any event that the user generates by interacting with the client
            app, usually by clicking on buttons or typing. This observable will emit every time
            the desired action type (e.g. login request, signout request) is detected.

        Usage:
            const loginAction = action(ActionType.LOGIN).pipe(
                tap(x => console.log("x"))
            )

            // Output: {email: test@test.com, password: Test123}

        Arguments:
            children(JSX.Element | JSX.Element[]): Any children nested within this component
    */

  // Helper function which takes an Action-emitting observable and emits only Actions that match
  // the type passed in above
  const filterType = (observable: Observable<Action>) =>
    observable.pipe(
      filter((x) => x !== undefined && x.type === type),
      map((x) => x.payload)
    )

  /* eslint-disable @typescript-eslint/strict-boolean-expressions */
  if ((<any>Object).values(RendererAction).includes(type))
    return filterType(fromEventIPC("action"))

  return filterType(eventTray)
}

// Should all actions be "hot"?
export const loginAction = action(RendererAction.LOGIN)
export const signupAction = action(RendererAction.SIGNUP)

export const signoutAction = action(TrayAction.SIGNOUT).pipe(share())
export const quitAction = action(TrayAction.QUIT).pipe(share())

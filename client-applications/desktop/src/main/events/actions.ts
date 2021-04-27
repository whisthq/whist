import { fromEventIPC } from "@app/main/events/ipc"
import { eventTray } from "@app/main/events/tray"
import { Observable } from "rxjs"
import { filter, map } from "rxjs/operators"
import { ActionType, RendererAction, Action } from "@app/@types/actions"

export const action = (
  type: ActionType,
  filterPayloadBy?: string
): Observable<any> => {
  const filterType = (observable: Observable<Action>) =>
    observable.pipe(
      filter((x) => x !== undefined && x.type === type),
      map((x) => x.payload),
      map((x) =>
        filterPayloadBy !== undefined && x !== null ? x[filterPayloadBy] : x
      )
    )

  /* eslint-disable @typescript-eslint/strict-boolean-expressions */
  if ((<any>Object).values(RendererAction).includes(type))
    return filterType(fromEventIPC("action"))

  return filterType(eventTray)
}

import { Observable, fromEvent, Subscription, Subject, of } from "rxjs"
import { ipcMain, BrowserWindow } from "electron"
import { filter, share, pluck, exhaustMap, scan } from "rxjs/operators"
import { nestMapValues, nestValues, nestMerge } from "@app/utils/nest"
import { mapValues, isUndefined, negate } from "lodash"
import { createAuthWindow } from "@app/utils/windows"

type FlowTree = {
  [parent: string]: (trigger: Observable<any>) => {
    [child: string]: Observable<any>
  }
}

interface Effect<T extends Observable<any>, F extends FlowTree> {
  (trigger: T, tree: F): { [P in keyof F]: ReturnType<F[P]> }
}

export const createWindowEffect = <
  T extends Observable<any>,
  F extends FlowTree,
  B extends BrowserWindow,
  R extends ReturnType<Effect<T, F>>
>(
  trigger: T,
  flowTree: F,
  callback: (flows: R, window: B) => void,
  ipcChannel: string,
  windowFn: (name: string) => B
) => {
  // These two subjects will emit data as it's ready to be sent over ipc,
  // or as it is received through ipc.
  const ipcSend = new Subject<{ [key: string]: any }>()
  const ipcReceive = new Subject<{ [key: string]: any }>()

  const result = mapValues(flowTree, (flowFn, key) =>
    flowFn(ipcReceive.pipe(pluck(key), filter(negate(isUndefined)), share()))
  ) as R

  trigger
    .pipe(
      exhaustMap(() => {
        const ipcListener = fromEvent(ipcMain, ipcChannel) as Observable<any>
        const window = windowFn(ipcChannel)

        // In case there are no upstream subscribers, we manually subscribe
        // to all the channels here. So that child flows still fire with
        // window activity. We'll unsubscribe from these when the window is
        // closed.
        const subs = nestMapValues(
          result,
          (value: Observable<any>, [parentKey, childKey]) =>
            value.subscribe((value: any) =>
              ipcSend.next({ [parentKey]: { [childKey]: value } })
            )
        )

        window.webContents.on("did-finish-load", () => {
          // Set up ipc channels to communicate with renderer
          // The "send" subscription collects reduces its previous states
          // into a single object with a merge.
          const ipcSendSub = ipcSend
            .pipe(scan((acc, value) => nestMerge(acc, value)))
            .subscribe((state) => window.webContents.send(ipcChannel, state))
          // The "receive" subscription sends new events from the ipc channel
          // onto to the ipcReceive subject above. We do this so that we can
          // reference the ipcReceive subject earlier in this function.
          const ipcReceiveSub = ipcListener.subscribe(([_event, data]) => {
            ipcReceive.next(data)
          })
          // Perform some cleanup when the window closes. Send a signal to
          // a closeSignal subject that we've set up, and unsubscribe from
          //
          // ipc subscriptions +
          window.on("closed", () => {
            ipcSendSub.unsubscribe()
            ipcReceiveSub.unsubscribe()
            nestValues(subs).forEach((s) => (s as Subscription).unsubscribe())
          })
        })

        // We'll return the window object along with all the results to
        // pass over to a callback.
        return of([result, window] as [R, B])
      })
    )
    .subscribe(([result, window]) => callback(result, window))

  return result
}

export const effect = <T extends Observable<any>, F extends FlowTree>(
  name: string,
  fn: Effect<T, F>
) => {
  console.log("FIRING EFFECT:", name)
  return fn
}

export const authWindowEffect = effect("authWindowEffect", (trigger, tree) =>
  createWindowEffect(
    trigger,
    tree,
    ({ login, signup }, window) => {
      login.success.subscribe(() => window.close())
      signup.success.subscribe(() => window.close())
    },
    "MAIN_STATE_CHANNEL",
    () => createAuthWindow()
  )
)

// export const authWindowEffect: EffectFn = (trigger, flowTree) =>
//   createWindowEffect(trigger, flowTree)

// export const withEffects = <T extends Observable<any>, F extends FlowTree>(
//   trigger: T,
//   tree: F,
//   effect: Effect<T, F>
// ) => effect(trigger, tree, {})

import { ReplaySubject } from "rxjs"
import { share, filter } from "rxjs/operators"

import { MandelboxState } from "@app/constants/storage"
import onChange from "on-change"
import { MandelboxInfo, HostInfo } from "@app/@types/payload"

const _stateDidChange = new ReplaySubject()

const _whistState = {
  waitingCloudTabs: [] as chrome.tabs.Tab[],
  activeCloudTabs: [] as chrome.tabs.Tab[],
  mandelboxState: MandelboxState.MANDELBOX_NONEXISTENT as MandelboxState,
  mandelboxInfo: undefined as (HostInfo & MandelboxInfo) | undefined,
  alreadyAddedCookies: [] as chrome.cookies.Cookie[],
}

const whistState = onChange(
  _whistState,
  (
    path: string,
    value: any,
    previousValue: any,
    applyData: {
      name: string
      args: any[]
    }
  ) => {
    _stateDidChange.next({
      path,
      value,
      previousValue,
      applyData,
    })
  }
)

const stateDidChange = (state: string) =>
  _stateDidChange.pipe(
    filter(({ path }) => path === state),
    share()
  )

export { whistState, stateDidChange }

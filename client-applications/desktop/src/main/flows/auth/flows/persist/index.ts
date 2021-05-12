import {
  store,
  persist,
  onPersistChange,
} from "@app/main/flows/auth/flows/persist/utils"
import { BehaviorSubject } from "rxjs"
import { switchMap } from "rxjs/operators"
import { has, every } from "lodash"

import { flow, fork } from "@app/utils/fork"

const persistStore = new BehaviorSubject(store.store)
onPersistChange((store: any) => persistStore.next(store))

store.onDidAnyChange((newStore: any, _oldStore: any) => {
  persistStore.next(newStore)
})

const persistedCredsValid = (store: any) =>
  every(["email", "accessToken", "configToken", "refreshToken"], (key) =>
    has(store, key)
  )

export default flow("persistedFlow", (_, trigger) =>
  fork(trigger.pipe(switchMap(() => persistStore)), {
    success: (store) => persistedCredsValid(store),
    failure: (store) => !persistedCredsValid(store),
  })
)

import { Dispatch as ReduxDispatch, Store as ReduxStore, Action } from 'redux'

export type mainStateType = any

export type Dispatch = ReduxDispatch<Action<string>>

export type Store = ReduxStore<mainStateType, Action<string>>

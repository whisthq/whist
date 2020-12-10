import { Dispatch as ReduxDispatch, Store as ReduxStore, Action } from "redux"

export type mainStateType = any

export type Dispatch = ReduxDispatch<Action<string>>

export type Store = ReduxStore<mainStateType, Action<string>>

export type ExternalAppType = {
    code_name: string
    display_name: string
    image_s3_uri: string
    tos_uri: string
}

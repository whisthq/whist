import { Dispatch as ReduxDispatch, Store as ReduxStore, Action } from 'redux'

export type mainStateType = {
    username: string
    public_ip: string
    warning: boolean
    os: string
    container_id: string
    cluster: string
    port_32262: string
    port_32263: string
    port_32273: string
    location: string
    access_token: string
    refresh_token: string
    attach_attempts: number
    account_locked: boolean
    promo_code: string
    status_message: string
    percent_loaded: number
}

export type Dispatch = ReduxDispatch<Action<string>>

export type Store = ReduxStore<mainStateType, Action<string>>

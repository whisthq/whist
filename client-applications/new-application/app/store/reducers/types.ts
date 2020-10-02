import { Dispatch as ReduxDispatch, Store as ReduxStore, Action } from 'redux'

export type mainStateType = {
    username: string
    publicIP: string
    warning: boolean
    os: string
    container_id: string
    cluster: string
    port32262: string
    port32263: string
    port32273: string
    location: string
    accessToken: string
    refreshToken: string
    accountLocked: boolean
    promoCode: string
    statusMessage: string
    percentLoaded: number
}

export type Dispatch = ReduxDispatch<Action<string>>

export type Store = ReduxStore<mainStateType, Action<string>>

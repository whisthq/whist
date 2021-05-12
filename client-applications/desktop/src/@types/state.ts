import { Observable } from "rxjs"
import { Action } from "@app/@types/actions"
import { Object } from "lodash"

export type AsyncReturnType<
  T extends (...args: any) => Promise<any>
> = T extends (...args: any) => Promise<infer R> ? R : any

export interface StateIPC {
  email: string
  password: string
  loginWarning: string
  loginLoading: boolean
  signupWarning: string
  signupLoading: boolean
  errorRelaunchRequest: number
  updateInfo: string
  action: Action
}

export type FlowEvent = {
  name: string,
  payload: object
}

export type FlowReturnType = {
  [key: string]: Observable<FlowEvent>
}

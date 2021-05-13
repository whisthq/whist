import { Observable } from "rxjs"
import { Trigger } from "@app/main/utils/flows"
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
  trigger: Trigger
}

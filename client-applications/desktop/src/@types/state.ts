import { Trigger } from "@app/utils/flows"

export type AsyncReturnType<T extends (...args: any) => Promise<any>> =
  T extends (...args: any) => Promise<infer R> ? R : any

export interface StateIPC {
  email: string
  sub: string
  password: string
  refreshToken: string
  accessToken: string
  userConfigToken: string
  updateInfo: string
  trigger: Trigger
}

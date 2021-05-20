import { Trigger } from "@app/utils/flows"

export type AsyncReturnType<
  T extends (...args: any) => Promise<any>
> = T extends (...args: any) => Promise<infer R> ? R : any

export interface StateIPC {
  email: string
  password: string
  updateInfo: string
  trigger: Trigger
}

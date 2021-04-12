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
  loginRequest: { email: string, password: string }
  signupRequest: { email: string, password: string }
  errorRelaunchRequest: number
  updateInfo: string
}

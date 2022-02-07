import { Trigger } from "@app/main/utils/flows"

export type AsyncReturnType<T extends (...args: any) => Promise<any>> =
  T extends (...args: any) => Promise<infer R> ? R : any

export interface StateIPC {
  userEmail: string // User email returned by Auth0
  trigger: Trigger // Renderer triggers like button clicks
  appEnvironment: string // dev, staging, prod
  updateInfo: string // emitted by autoUpdater
  browsers: string[]
  networkInfo: { jitter: number; downloadMbps: number; progress: number }
  isDefaultBrowser: boolean
  restoreLastSession: boolean
  otherBrowserWindows: string[][]
  allowNonUSServers: boolean
}

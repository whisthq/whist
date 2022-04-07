import { Trigger } from "@app/main/utils/flows"
import { AWSRegion } from "@app/@types/aws"

export type AsyncReturnType<T extends (...args: any) => Promise<any>> =
  T extends (...args: any) => Promise<infer R> ? R : any

export interface StateIPC {
  userEmail: string // User email returned by Auth0
  subscriptionStatus: string
  appEnvironment: string // dev, staging, prod
  updateInfo: string // emitted by autoUpdater
  browsers: string[]
  networkInfo: {
    jitter: number
    downloadMbps: number
    progress: number
    ping: number
  }
  isDefaultBrowser: boolean
  restoreLastSession: boolean
  otherBrowserWindows: string[][]
  allowNonUSServers: boolean
  platform: string
  regions: AWSRegion[]
  trigger: Trigger // Renderer triggers like button clicks
}

import { Trigger } from "@app/utils/flows"

export type AsyncReturnType<T extends (...args: any) => Promise<any>> =
  T extends (...args: any) => Promise<infer R> ? R : any

export interface StateIPC {
  userEmail: string // User email returned by Auth0
  password: string // Currently deprecated
  refreshToken: string // Auth0 refresh token
  accessToken: string // JWT
  subscriptionStatus: string // user's current Stripe subscription status
  userConfigToken: string // Config token for app config security
  updateInfo: string // Autoupdate object with update stats like download speed, sent to render thread
  trigger: Trigger // Renderer triggers like button clicks
}

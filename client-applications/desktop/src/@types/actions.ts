export enum MainAction {
  SIGNOUT = "signoutRequest",
  QUIT = "quitRequest",
}

export enum RendererAction {
  LOGIN = "loginRequest",
  SIGNUP = "signupRequest",
}

export enum PaymentAction {
  CHECKOUT = "stripeCheckoutRequest",
  PORTAL = "stripePortalRequest"
}

export type ActionType = MainAction | RendererAction | PaymentAction

export interface Action {
  type: ActionType
  payload: Record<string, any> | null
}

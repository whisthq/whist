export enum TrayAction {
  SIGNOUT = "signoutRequest",
  QUIT = "quitRequest",
}

export enum RendererAction {
  LOGIN = "loginRequest",
  SIGNUP = "signupRequest",
}

export type ActionType = TrayAction | RendererAction

export interface Action {
  type: ActionType
  payload: Record<string, any> | null
}

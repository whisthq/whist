export enum ActionType {
  LOGIN = "loginRequest",
  SIGNUP = "signupRequest",
  SIGNOUT = "signoutRequest",
  QUIT = "quitRequest",
}

export interface Action {
  type: ActionType
  payload: Record<string, any> | null
}

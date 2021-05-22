export interface FractalError {
  hash: string
  title: string
  text: string
}

export const NoAccessError: FractalError = {
  hash: "CREATE_CONTAINER_ERROR_NO_ACCESS",
  title: "Your account does not have access to Fractal.",
  text: "Access to Fractal is currently invite-only. Please contact support@fractal.co for help.",
}

export const UnauthorizedError: FractalError = {
  hash: "CREATE_CONTAINER_ERROR_UNAUTHORIZED",
  title: "There was an error authenticating you with Fractal.",
  text: "Please try logging in again or contact support@fractal.co for help.",
}

export const ProtocolError: FractalError = {
  hash: "PROTOCOL_ERROR",
  title: "The Fractal browser encountered an unexpected error.",
  text: "Please try again in a few minutes or contact support@fractal.co for help.",
}

export const ContainerError: FractalError = {
  hash: "CREATE_CONTAINER_ERROR_INTERNAL",
  title: "There was an error connecting to the Fractal servers.",
  text: "Please try again in a few minutes or contact support@fractal.co for help.",
}

export const AuthError: FractalError = {
  hash: "AUTH_ERROR",
  title: "There was an error logging you in",
  text: "Please try logging in again or contact support@fractal.co for help.",
}

export const NavigationError: FractalError = {
  hash: "NAVIGATION_ERROR",
  title: "There was an error loading the Fractal window.",
  text: "Please try logging in again or contact support@fractal.co for help.",
}
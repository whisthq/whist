import { BrowserWindowConstructorOptions } from "electron"

import { createWindow, base, width, height } from "@app/utils/windows"

export type FractalError = {
  hash: string
  title: string
  message: string
}

export const NoAccessError: FractalError = {
  hash: "CONTAINER_ERROR_NO_ACCESS",
  title: "Your account does not have access to Fractal.",
  message:
    "Access to Fractal is currently invite-only. Please contact support@fractal.co for help.",
}

export const UnauthorizedError: FractalError = {
  hash: "CONTAINER_ERROR_UNAUTHORIZED",
  title: "There was an error authenticating you with Fractal.",
  message:
    "Please try logging in again or contact support@fractal.co for help.",
}

export const ProtocolError: FractalError = {
  hash: "PROTOCOL_ERROR",
  title: "The Fractal browser encountered an unexpected error.",
  message:
    "Please try again in a few minutes or contact support@fractal.co for help.",
}

export const ContainerError: FractalError = {
  hash: "CONTAINER_ERROR",
  title: "There was an error connecting to the Fractal servers.",
  message:
    "Please try again in a few minutes or contact support@fractal.co for help.",
}

export const AuthError: FractalError = {
  hash: "AUTH_ERROR",
  title: "There was an error logging you in",
  message:
    "Please try logging in again or contact support@fractal.co for help.",
}

export const NavigationError: FractalError = {
  hash: "NAVIGATION_ERROR",
  title: "There was an error loading this window.",
  message: "Please restart Fractal or contact support@fractal.co for help.",
}

export const createErrorWindow = (error: FractalError) => {
  createWindow(error.hash, {
    ...base,
    ...width.md,
    ...height.xs,
  } as BrowserWindowConstructorOptions)
}
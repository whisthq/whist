import { initWhistAuthHandler, initGoogleAuthHandler } from "./handlers/auth"

// Auth handling on every launch
initWhistAuthHandler()

// Opens Google auth window
initGoogleAuthHandler()

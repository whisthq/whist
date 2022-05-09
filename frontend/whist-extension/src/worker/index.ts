import { initWhistAuthHandler, initGoogleAuthHandler } from "./handlers/auth"
import FuzzySearch from "fuzzy-search"

// Auth handling on every launch
initWhistAuthHandler()

// Opens Google auth window
initGoogleAuthHandler()

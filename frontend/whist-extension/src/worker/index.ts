import { initWhistAuthHandler, initGoogleAuthHandler } from "./handlers/auth"
import { initMandelboxAssign } from "./handlers/mandelbox"
import { initAWSRegionPing } from "./handlers/location"
import { initHostSpinUp } from "./handlers/host"

// Auth handling on every launch
initWhistAuthHandler()

// Opens Google auth window
initGoogleAuthHandler()

// Finds the closest AWS regions
initAWSRegionPing()

// Sends mandelbox assign request
initMandelboxAssign()

// Tells the host service to spin up a mandelbox
initHostSpinUp()

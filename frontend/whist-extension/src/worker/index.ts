import { initWhistAuthHandler, initGoogleAuthHandler } from "./auth"
import { initMandelboxAssign } from "./mandelbox"
import { initHostSpinUp } from "./host"
import { initAWSRegionPing } from "./location"

// Auth handling on every launch
initWhistAuthHandler()

// Opens Google auth window
initGoogleAuthHandler()

// Ping all AWS regions
initAWSRegionPing()

// Sends mandelbox assign request
initMandelboxAssign()

// Tells the host service to spin up a mandelbox
initHostSpinUp()

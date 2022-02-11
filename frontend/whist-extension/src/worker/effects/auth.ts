import { openGoogleAuth, shouldAuthenticate } from "@app/worker/events/auth"
import { googleAuth, authenticate } from "@app/worker/flows/auth"

console.log("Auth effects starting")

// Checks to see if the user is authenticated
shouldAuthenticate.subscribe(() => authenticate())
// Opens the Google Auth popup
openGoogleAuth.subscribe(() => googleAuth())

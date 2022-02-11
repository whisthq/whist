import { merge } from "rxjs"

import { openGoogleAuth, shouldAuthenticate } from "@app/worker/events/auth"
import { storageDidChange } from "@app/worker/events/storage"

import { googleAuth, authenticate } from "@app/worker/flows/auth"
import { createLoggedInTab } from "@app/worker/utils/tabs"

// Checks to see if the user is authenticated
merge(shouldAuthenticate, storageDidChange).subscribe(async () => {
  const authenticated = await authenticate()
  if (authenticated) createLoggedInTab()
})
// Opens the Google Auth popup
openGoogleAuth.subscribe(() => {
  googleAuth()
})

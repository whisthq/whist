import { merge } from "rxjs"
import { switchMap } from "rxjs/operators"

import { openGoogleAuth, shouldAuthenticate } from "@app/worker/events/auth"
import { storageDidChange } from "@app/worker/events/storage"

import { googleAuth, authenticate } from "@app/worker/flows/auth"
import { createEvent } from "@app/worker/utils/events"

// Checks to see if the user is authenticated
merge(shouldAuthenticate, storageDidChange).subscribe(async () => {
  const authenticated = await authenticate()
  if (authenticated) createEvent("authenticated")
})
// Opens the Google Auth popup
openGoogleAuth.subscribe(() => {
  googleAuth()
})

import { appReady, ipcState } from "@app/main/events"
import { createAuthWindow, closeWindows } from "@app/utils/windows"
import { loginSuccess } from "@app/main/login"
import { emailLoginAccessToken } from "@app/utils/api"
import { zip, of, NEVER } from "rxjs"
import { map, pluck, distinctUntilChanged } from "rxjs/operators"

export const userEmail = ipcState.pipe(pluck("email"), distinctUntilChanged())

export const userAccessToken = loginSuccess.pipe(map(emailLoginAccessToken))

// Start the auth window on launch
appReady.subscribe(() => {
    createAuthWindow((win: any) => win.show())
})

// If we have an access token and an email, close the existing windows.
zip(userEmail, userAccessToken).subscribe(() => {
    closeWindows()
})

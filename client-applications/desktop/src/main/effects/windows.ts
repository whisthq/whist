import { app, IpcMainEvent } from "electron"
import { takeUntil, withLatestFrom, startWith, mapTo } from "rxjs/operators"
import { merge } from "rxjs"

import { destroyTray } from "@app/utils/tray"
import { uploadToS3 } from "@app/utils/logging"
import { fromTrigger } from "@app/utils/flows"
import { WindowHashProtocol } from "@app/utils/constants"
import { persistGet } from "@app/utils/persist"
import { createTypeformWindow } from "@app/utils/windows"

fromTrigger("windowsAllClosed")
    .pipe(
        takeUntil(
            merge(
                fromTrigger("updateDownloaded"),
                fromTrigger("updateAvailable")
            )
        )
    )
    .subscribe((evt: IpcMainEvent) => {
        evt?.preventDefault()
    })

fromTrigger("windowInfo")
    .pipe(
        takeUntil(
            merge(
                fromTrigger("updateDownloaded"),
                fromTrigger("updateAvailable")
            )
        ),
        withLatestFrom(
            fromTrigger("mandelboxFlowFailure").pipe(
                mapTo(true),
                startWith(false)
            )
        )
    )
    .subscribe(
        ([args, mandelboxFailed]: [
            {
                numberWindowsRemaining: number
                crashed: boolean
                hash: string
            },
            boolean
        ]) => {
            // If there are still windows open, ignore
            if (args.numberWindowsRemaining !== 0) return
            // If all windows are closed and the protocol wasn't the last open window, quit
            if (args.hash !== WindowHashProtocol) {
                app.quit()
                return
            }
            // If the protocol was the last window to be closed and we haven't asked for feedback,
            // ask for feedback
            if (
                !(
                    (persistGet(
                        "typeformFeedbackSubmitted",
                        "data"
                    ) as boolean) ?? false
                ) &&
                !args.crashed &&
                !mandelboxFailed
            ) {
                createTypeformWindow()
                return
            }
            // If the protocol was the last window to be closed, upload logs and quit the app
            destroyTray()
            uploadToS3()
                .then(() => {
                    if (!args.crashed) app.quit()
                })
                .catch((err) => {
                    console.error(err)
                    if (!args.crashed) app.quit()
                })
        }
    )

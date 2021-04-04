import { merge, from, of, fromEvent, race, interval } from "rxjs"
import {
    mapTo,
    map,
    share,
    switchMap,
    exhaustMap,
    filter,
    catchError,
} from "rxjs/operators"
import {
    eventUpdateCheck,
    eventUpdateNotAvailable,
    eventUpdateAvailable,
} from "@app/events/autoupdate"

// These are fake functions, they don't belong here at all, but an example
// of some possible utility functions we might need to plug into our observables
// below.
const updateDownload = () => Promise.all([])
const updateDownloadProgress = () => null
const updateDownloadValidate = () => null

// Any time an update is available, we request the download.
// Assuming downloadUpdate returns a promise here.
// share() is used here because we have a side effect. Default observable
// behavior is to re-run for every subscriber, so updateDownload would run
// once for each subscriber, which we don't want. With share(), we make sure
// that each subscriber "shares" the result of autoUpdateDownloadRequest, rather
// than re-doing the computation.
export const autoUpdateDownloadRequest = eventUpdateAvailable.pipe(
    map(() => updateDownload()),
    share()
)

// This observable emits the download progress, it depends on the implementation
// of the download functions, but this example just checks the size of the
// downloading file every 100ms. We would probably subscribe to this in
// effects/ipc.ts to send the download status to the renderer thread.
export const autoUpdateDownloadPolling = autoUpdateDownloadRequest.pipe(
    switchMap(() => interval(100).pipe(map(() => updateDownloadProgress())))
)

// Assuming autoUpdateDownloadRequest emits a promise, using rxjs "from"
// will convert the promise to an observable.
// exhaustMap is a higher-order observable operator, it takes a function that
// returns an observable (similar to switchMap, mergeMap, and flatMap).
// "exhaust" means that we're waiting for the "from(promise)" to complete,
// and ignoring anything else that comes along before it's complete.
// In case that multiple autoUpdateDownloadRequests were triggered for some
// reason, autoUpdateDownloadFailure would only report the status of the
// first one to be triggered, ignoring future ones. After the first one is
// complete, it would accept the next one to come along. Of course, this isn't
// the expected behavior of our download flow at all, but rxjs helps us think
// through unexpected behavior like this.
export const autoUpdateDownloadFailure = autoUpdateDownloadRequest.pipe(
    exhaustMap((promise) => from(promise as Promise<any>)),
    catchError((err) => of(err)),
    filter(() => !updateDownloadValidate())
)

// We would probably have a subscription in effects/autoupdate.ts that
// waits for this success observable, and then calls quitAndInstall().
export const autoUpdateDownloadSuccess = autoUpdateDownloadRequest.pipe(
    exhaustMap((promise) => from(promise as Promise<any>)),
    filter(() => !updateDownloadValidate())
)

// The biggest learning curve in rxjs is thinking about higher-order observables,
// but they're super powerful and concise. Here, exhaustMap is performing the
// same function as it did in "failure" and "success", only responding one
// request at a time. It will emit "true" as soon as we see a
// autoUpdateDownloadRequest, and then switch to false as soon as we see either
// success or failure.
export const autoUpdateDownloadLoading = autoUpdateDownloadRequest.pipe(
    exhaustMap(() =>
        merge(
            of(true),
            autoUpdateDownloadFailure.pipe(mapTo(false)),
            autoUpdateDownloadSuccess.pipe(mapTo(false))
        )
    )
)

// The observables above only handle the actual file downloading.
// The rest of the app probably cares more about the overall update state,
// so here's some ideas for some observables that would look at the bigger
// picture.

// This observable would fire every time when the app starts as we're checking.
export const autoUpdateRequest = eventUpdateCheck

// We could reference this observable to help decide what window to show
// when the application starts. Essentially, the point of a update is to
// put us in a state where no update is available. So autoUpdateSuccess
// should be true in any case where we're in that state

// Because quitting and re-launching the app is part of our update flow,
// when the app restarts we should expect this to be true.
export const autoUpdateSuccess = eventUpdateNotAvailable

// This is a naive implementation, there's probably more sophisticated failure
// handling that we should do. But in this example, the only thing that causes
// a update failure is a download failure, so I'm just making the observables
// equivalent.
export const autoUpdateFailure = autoUpdateDownloadFailure

// I've realized that as long as we keep this pattern going of
// "request, polling, success, failure" (which seems to work for a lot!),
// that the "loading" observables are always pretty much the exact same code.
// We should make a utils/observables.ts file or something that can house
// useful observable creating functions like this that we reuse all the time.
export const autoUpdateLoading = autoUpdateRequest.pipe(
    switchMap(function () {
        return merge(
            of(true),
            autoUpdateSuccess.pipe(mapTo(false)),
            autoUpdateFailure.pipe(mapTo(false))
        )
    })
)

import "@app/main/triggers/app"
import "@app/main/triggers/auth"
import "@app/main/triggers/autoupdate"
import "@app/main/triggers/ipc"
import "@app/main/triggers/persist"
import "@app/main/triggers/power"
import "@app/main/triggers/renderer"
import "@app/main/triggers/tray"
import "@app/main/triggers/payment"

// Events tend to be event listeners on processes that are
// outside the main thread's control, like Electron application events. These
// event listeners are made into observables, which are subscribed to
// throughout the application.
//
// One kind of "event" is communication from the renderer thread, which
// is made up the GUI windows that the user interacts with. All communication
// with the renderer thread is done through a single channel, and we process
// new messages, which are in the shape of a state object, as an "event" here.
//
// Importantly, this file also sets up an observable that listens to the
// persisted local storage. Local storage is the "source of truth" for many
// of the app's subscribers, particularly for user data.
//
// This file represents the "begining" of the application's reactive data
// flow. The observables in this file are "event sources", so they should not
// subscribe to any observables in the rest of the application. Subscribing
// to a "downstream" observable from here can cause circular dependencies.

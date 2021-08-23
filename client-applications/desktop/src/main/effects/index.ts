import "@app/main/effects/app"
import "@app/main/effects/autolaunch"
import "@app/main/effects/error"
import "@app/main/effects/ipc"
import "@app/main/effects/persist"
import "@app/main/effects/protocol"
import "@app/main/effects/windows"

// Effects are subscriptions that cause side effects throughout
// the program. Any observable from any other file in the application
// can be imported and subscribed to here. Opening and closing windows,
// quitting the application, streaming information to the protocol,
// and persisting data to local storage are examples.
//
// We keep these effects in a single file to isolate them from the
// more "pure" observables in the rest of the application. As a rule of
// thumb, if you're using the .subscribe() method on a observable, it
// has a side effect and it belongs in this file.
//
// We differentiate these effects from regular side effects based on whether
// or not we care to check their return value. We don't consider loginEmail
// an "Effect" in this sense, even though it is not a pure function.
// We care very much about the result of loginEmail, and important behavior is
// triggered based on the contents of the loginEmail result.
// While loginEmail happens to get its result from our webserver and
// is thus async, observables do not discriminate their handling of
// async functions.
//
// Because nothing is reacting to their results, these functions represent
// the "end" of the reactive data flow. Other files in the application should
// not subscribe to any of these Effects. If something "upstream" tries to
// subscribe to any Effect, it can cause circular dependencies.

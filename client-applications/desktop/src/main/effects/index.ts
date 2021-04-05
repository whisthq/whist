// Effects are subscriptions that cause side effects throughout
// the program. Any observable from any other file in the application
// can be imported and subscribed to here.
//
// Side effects in this file include anything that doesn't return a
// response when called. Opening and closing windows, quitting the
// application, streaming information to the protocol, and persisting
// data to local storage are examples.
//
// We keep these effects in a single file to isolate them from the
// more "pure" observables in the rest of the application. As a rule of
// thumb, if you're using the .subscribe() method on a observable, it
// has a side effect and it belongs in this file.
//
// This file represents the "end" of the reactive data flow. Other files
// in the application should not subscribe to anything in this file. If
// something "upstream" subscribes to any observable here, it can cause
// circular dependencies.

import "@app/main/effects/app"
import "@app/main/effects/error"
import "@app/main/effects/ipc"
import "@app/main/effects/persist"
import "@app/main/effects/protocol"
import "@app/main/effects/logging"


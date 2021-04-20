import '@app/main/observables/container'
import '@app/main/observables/error'
import '@app/main/observables/host'
import '@app/main/observables/login'
import '@app/main/observables/protocol'
import '@app/main/observables/signup'
import '@app/main/observables/user'
import '@app/main/observables/signout'

// As with other 'index.ts' files in the main process, we only intialize the
// module's imports here. No code lives in this file, as the observables in the
// imported files manage their own subscriptions.

// The observables in this folder should follow a strict set of conventions
// for naming and processing. They have a defined vocabulary to describe their
// various phases of computation and results. Any new observables should adhere
// to this naming scheme.
//
// These stages are all optional, any number of them can be employed.

// request: Subscribes to a upstream observable that should "trigger" certain
//          behavior. The job of "request" is to decide whether to process this
//          trigger, and if so extract any data required for processing. The
//          emitted value of "request" should be the exact arguments required
//          for the function that actually performs the work.
//
// process: Takes the arguments emitted by "request", and calls a function that
//          actually performs the work. "process" should emit the result of the
//          function, not caring whether or not the function is async. "process"
//          should not transform the result, emitting "as is". It may catch
//          errors and return them.
//
// polling: Performs a function that monitors the progress of the "process"
//          function. This can be useful if the "process" has a side effect,
//          like changing webserver state or downloading a file, and that
//          progress needs to be monitored. The "polling" observable should
//          close when it decides that progress has completed or failed.
//
// warning: A completion state that represents an invalid result, but is not
//          so critical as to cause an application crash. An example might be
//          an unsuccessful login attempt, where the user should be free to
//          try again. It's expected that if "warning" fires, "failure" and
//          "success" will not fire. The emitted data should be the
//          untransformed result of "process".
//
// success: A completion state that represents a successful result. It is
//          expected that downstream observables will subscribe to "success" if
//          they require the result of this process. It's expected that if
//         "success" fires, "failure" and "warning" will not fire. The emitted
//         data should be the untransformed result of "process".
//
// failure: A completion state that represents an error in the result. It is
//          expected that this will trigger an application crash and display
//          an error to the user. It's expected that if "failure" fires,
//         "success" and "warning" will not fire. The emitted data should be
//          the untransformed result of "process".
//
// loading: Subscribes to the results of "request", and any completion states.
//          It emits "true" after "request", and then emits "false" after
//          an event from any completion state.

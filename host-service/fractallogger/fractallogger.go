/*
Package fractallogger contains the logic for our custom logging system, including sending events to Logz.io and Sentry.
*/
package fractallogger // import "github.com/fractal/fractal/host-service/fractallogger"

import (
	"context"
	"fmt"
	"log"
	"runtime/debug"
	"time"

	"github.com/fractal/fractal/host-service/utils"
)

func init() {
	// Set some options for the Go logging package.
	log.Default().SetFlags(log.Ldate | log.Lmicroseconds | log.LUTC)

	// The first thing we want to do is to initialize logzio and Sentry so that
	// we can catch any errors that might occur, or logs if we print them.
	// The credentials for Sentry are hardcoded into the host-service,
	// and the credentials for logzio are written to the environment file used
	// by host-service systemd service.
	// It is more likely that logzio initialization would fail, So we initialize
	// Sentry first.

	// We declare error separately to avoid shadowing sentryTransport.
	var err error

	// Note that according to the Sentry Go documentation, if we run
	// sentry.CaptureMessage or sentry.CaptureError on separate goroutines, they
	// can overwrite each other's tags. The thread-safe solution is to use
	// goroutine-local hubs, but the way to do that would be to use contexts and
	// add an additional argument to each logging method taking in the current
	// context. This seems like a lot of work, so we just use a set of global
	// tags instead, initializing them in InitializeSentry() and not touching
	// them afterwards. Any mandelbox-specific information (which is what I
	// imagine we would use local tags for) we just add in the text of the
	// respective error message sent to Sentry. Alternatively, we might just be
	// able to use sentry.WithScope(), but that is future work.
	sentryTransport, err = initializeSentry()
	if err != nil {
		// Error, don't Panic, since a Sentry outage should not bring down our
		// entire service.
		Errorf("Failed to initialize Sentry! Error: %s", err)
	}

	logzioTransport, err = initializeLogzIO()
	if err != nil {
		// Error, don't Panic, since a logzio outage should not bring down our
		// entire service.
		Errorf("Failed to initialize LogzIO! Error: %s", err)
	}
}

// Close flushes all production logging (i.e. Sentry and Logzio).
func Close() {
	// Flush buffered logging events before the program terminates.
	Info("Flushing Sentry...")
	FlushSentry()
	Info("Flushing Logzio...")
	stopAndDrainLogzio()
}

// Info logs some info + timestamp, but does not send it to Sentry.
func Info(format string, v ...interface{}) {
	str := fmt.Sprintf(format, v...)

	log.Print(str)
	if logzioTransport != nil {
		timestamp := fmt.Sprintf("time=%s", time.Now().String())
		logzioTransport.send(fmt.Sprintf("%s %s", str, timestamp), logzioTypeInfo)
	}
}

// Error logs an error and sends it to Sentry.
func Error(err error) {
	errstr := fmt.Sprintf("ERROR: %s", err)
	log.Printf(utils.ColorRed(errstr))
	if logzioTransport != nil {
		logzioTransport.send(errstr, logzioTypeError)
	}
	if sentryTransport != nil {
		sentryTransport.send(err)
	}
}

// Warning logs an error in red text, like Error, but doesn't send it to
// Sentry.
func Warning(err error) {
	str := fmt.Sprintf("WARNING: %s", err)
	log.Printf(utils.ColorRed(str))
	if logzioTransport != nil {
		logzioTransport.send(str, logzioTypeWarning)
	}
}

// Panic sends an error to Sentry and "pretends" to panic on it by printing the
// stack trace and calling the provided global context-cancelling function.
// This causes all the goroutines in the program to kill themselves (cleanly).
// This function should not be used except to initiate termination of the
// entire host service. Note that passing in a nil first argument would cause
// this function to _actually_ panic, and if we're gonna panic we might as well
// do so in a useful way. Therefore, passing in a nil `globalCancel` parameter
// will just panic on `err` instead.
func Panic(globalCancel context.CancelFunc, err error) {
	errstr := fmt.Sprintf("PANIC: %s", err)
	if logzioTransport != nil {
		logzioTransport.send(errstr, logzioTypeError)
	}
	if sentryTransport != nil {
		sentryTransport.send(err)
	}
	PrintStackTrace()

	if globalCancel != nil {
		Error(err)
		globalCancel()
	} else {
		// If we're truly trying to panic, let's at least flush our logging queues
		// first so this error actually gets sent.
		FlushLogzio()
		FlushSentry()
		log.Panicf(utils.ColorRed(errstr))
	}
}

// Infof is identical to Info, since Info already respects printf syntax. We
// only include Infof for consistency with Errorf, Warningf, and Panicf.
func Infof(format string, v ...interface{}) {
	Info(format, v...)
}

// Errorf is like Error, but it respects printf syntax, i.e. takes in a format
// string and arguments, for convenience.
func Errorf(format string, v ...interface{}) {
	Error(utils.MakeError(format, v...))
}

// Warningf is like Warning, but it respects printf syntax, i.e. takes in a format
// string and arguments, for convenience.
func Warningf(format string, v ...interface{}) {
	Warning(utils.MakeError(format, v...))
}

// Panicf is like Panic, but it respects printf syntax, i.e. takes in a format
// string and arguments, for convenience.
func Panicf(globalCancel context.CancelFunc, format string, v ...interface{}) {
	Panic(globalCancel, utils.MakeError(format, v...))
}

// PrintStackTrace prints the stack trace, for debugging purposes.
func PrintStackTrace() {
	Info("Printing stack trace: ")
	debug.PrintStack()
}

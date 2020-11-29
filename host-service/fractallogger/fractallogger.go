package fractallogger

import (
	"fmt"
	"log"
	"runtime/debug"

	"github.com/getsentry/sentry-go"
)

// Create an error from format string and args
func MakeError(format string, v ...interface{}) error {
	return fmt.Errorf(format, v...)
}

// Create string from format string and args
func Sprintf(format string, v ...interface{}) string {
	return fmt.Sprintf(format, v...)
}

// Log an error and send it to sentry.
func Error(err error) {
	errstr := fmt.Sprintf("ERROR: %v", err)
	log.Println(errstr)
	sentry.CaptureException(err)
}

// Panic on an error. We do not send it to Sentry (to save on Sentry logging
// costs), since we do that when we recover from the panic in
// shutdownHostService(). Note that there are two types of panics that we are
// unable to send to Sentry using our current approach: panics in a goroutine
// that didn't `defer shutdownHostService()` as soon as it started, and panics
// caused by incorrect initialization of Sentry in the first place.
func Panic(err error) {
	log.Panic(err)
}

// Log some info, but do not send it to Sentry
func Info(format string, v ...interface{}) {
	str := fmt.Sprintf(format, v...)
	log.Print(str)
}

// We also create a pair of functions that respect printf syntax, i.e. take in
// a format string and arguments, for convenience
func Errorf(format string, v ...interface{}) {
	Error(MakeError(format, v...))
}

func Panicf(format string, v ...interface{}) {
	Panic(MakeError(format, v...))
}

// For consistency, we also include a variant for Info even though it already
// takes in a format string and arguments.
func Infof(format string, v ...interface{}) {
	Info(format, v...)
}

func PrintStackTrace() {
	Info("Printing stack trace: ")
	debug.PrintStack()
}

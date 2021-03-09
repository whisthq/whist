package fractallogger

import (
	"fmt"
	"log"
	"runtime/debug"

	"github.com/getsentry/sentry-go"
)

func init() {
	init_metadata()
}

// MakeError creates an error from format string and args.
func MakeError(format string, v ...interface{}) error {
	return fmt.Errorf(format, v...)
}

// Sprintf creates a string from format string and args.
func Sprintf(format string, v ...interface{}) string {
	return fmt.Sprintf(format, v...)
}

// Error logs an error and sends it to Sentry.
func Error(err error) {
	log.Println(fmt.Sprintf("ERROR: %s", err))
	if UsingProdLogging() {
		sentry.CaptureException(err)
	}
}

// Panic panics on an error. We do not send it to Sentry (to save on Sentry
// logging costs), since we do that when we recover from the panic in
// shutdownHostService(). Note that there are two types of panics that we are
// unable to send to Sentry using our current approach: panics in a goroutine
// that didn't `defer shutdownHostService()` as soon as it started, and panics
// caused by incorrect initialization of Sentry in the first place.
func Panic(err error) {
	log.Panic(err)
}

// Info logs some some info, but does not send it to Sentry.
func Info(format string, v ...interface{}) {
	str := fmt.Sprintf(format, v...)
	log.Print(str)
}

// Errorf is like Error, but it respects printf syntax, i.e. takes in a format
// string and arguments, for convenience.
func Errorf(format string, v ...interface{}) {
	Error(MakeError(format, v...))
}

// Panicf is like Panic, but it respects printf syntax, i.e. takes in a format
// string and arguments, for convenience.
func Panicf(format string, v ...interface{}) {
	Panic(MakeError(format, v...))
}

// Infof is identical to Info, since Info already respects printf syntax. We
// only include Infof for consistency with Errorf and Panicf.
func Infof(format string, v ...interface{}) {
	Info(format, v...)
}

// PrintStackTrace prints the stack trace, for debugging purposes.
func PrintStackTrace() {
	Info("Printing stack trace: ")
	debug.PrintStack()
}

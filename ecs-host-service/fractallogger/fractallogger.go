package fractallogger

import (
	"fmt"
	"log"
	"runtime/debug"
	"time"

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

// Log an error
func Error(err error) {
	errstr := fmt.Sprintf("ERROR: %v", err)
	log.Println(errstr)
}

// Panic on an error
func Panic(err error) {
	log.Panic(err)
}

func Info(format string, v ...interface{}) {
	log.Printf(format, v...)
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

func InitializeSentry() error {
	return sentry.Init(sentry.ClientOptions{
		Dsn:   "https://5f2dd9674e2b4546b52c205d7382ac90@o400459.ingest.sentry.io/5461239",
		Debug: true,
	})
}

func FlushSentry() {
	sentry.Flush(5 * time.Second)
}

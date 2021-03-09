package fractallogger

import (
	"fmt"
	"log"
	"runtime/debug"
)

func init() {
	init_metadata()

	// We declare error separately to avoid shadowing logzioSender.
	var err error
	logzioTransport, err = initializeLogzIO()
	if err != nil {
		Errorf("Failed to initialize LogzIO! Error: %s", err)
	}

	// Note that according to the Sentry Go documentation, if we run
	// sentry.CaptureMessage or sentry.CaptureError on separate goroutines, they
	// can overwrite each other's tags. The thread-safe solution is to use
	// goroutine-local hubs, but the way to do that would be to use contexts and
	// add an additional argument to each logging method taking in the current
	// context. This seems like a lot of work, so we just use a set of global
	// tags instead, initializing them in InitializeSentry() and not touching
	// them afterwards. Any container-specific information (which is what I
	// imagine we would use local tags for) we just add in the text of the
	// respective error message sent to Sentry. Alternatively, we might just be
	// able to use sentry.WithScope(), but that is future work.
	sentryTransport, err = initializeSentry()
	if err != nil {
		Errorf("Failed to initialize Sentry! Error: %s", err)
	}
}

// MakeError creates an error from format string and args.
func MakeError(format string, v ...interface{}) error {
	return fmt.Errorf(format, v...)
}

// Sprintf creates a string from format string and args.
func Sprintf(format string, v ...interface{}) string {
	return fmt.Sprintf(format, v...)
}

const (
	colorReset = "\033[0m"
	colorRed   = "\033[31m"
)

// Error logs an error and sends it to Sentry.
func Error(err error) {
	errstr := fmt.Sprintf("ERROR: %s", err)
	log.Printf("%s%s%s", colorRed, errstr, colorReset)
	if logzioTransport != nil {
		logzioTransport.Send([]byte(errstr))
	}
	if sentryTransport != nil {
		sentryTransport.send(err)
	}
}

// Panic panics on an error and sends it to Sentry.
func Panic(err error) {
	errstr := fmt.Sprintf("PANIC: %s", err)
	if logzioTransport != nil {
		logzioTransport.Send([]byte(errstr))
	}
	if sentryTransport != nil {
		sentryTransport.send(err)
	}
	PrintStackTrace()
	log.Panic(err)
}

// Info logs some some info, but does not send it to Sentry.
func Info(format string, v ...interface{}) {
	str := fmt.Sprintf(format, v...)
	log.Print(str)
	if logzioTransport != nil {
		logzioTransport.Send([]byte(str))
	}
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

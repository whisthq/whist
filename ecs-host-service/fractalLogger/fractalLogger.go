package fractalLogger

import (
	"fmt"
	"log"
	"runtime/debug"
)

func MakeError(format string, v ...interface{}) error {
	return fmt.Errorf(format, v...)
}

func Sprintf(format string, v ...interface{}) string {
	return fmt.Sprintf(format, v...)
}

func Error(err error) {
	errstr := fmt.Sprintf("ERROR: %v", err)
	log.Println(errstr)
}

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

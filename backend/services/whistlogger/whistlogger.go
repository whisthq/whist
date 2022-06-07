package whistlogger

import (
	"context"
	"io/ioutil"
	"os"
	"runtime/debug"

	"github.com/whisthq/whist/backend/services/utils"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

var logger *zap.Logger

func init() {
	// First, define our level-handling logic.
	highPriority := zap.LevelEnablerFunc(func(lvl zapcore.Level) bool {
		return lvl >= zapcore.ErrorLevel
	})
	lowPriority := zap.LevelEnablerFunc(func(lvl zapcore.Level) bool {
		return lvl < zapcore.ErrorLevel
	})

	debugging := zapcore.AddSync(ioutil.Discard)
	errors := zapcore.AddSync(ioutil.Discard)

	// High-priority output should go to standard error, and low-priority
	// output should also go to standard out.
	consoleDebugging := zapcore.Lock(os.Stdout)
	consoleErrors := zapcore.Lock(os.Stderr)

	sentryAndLogzEncoderConfig := zap.NewProductionEncoderConfig()
	consoleEncoderConfig := zap.NewDevelopmentEncoderConfig()

	// Enable colored output on stdout
	consoleEncoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder

	sentryAndLogzEncoder := zapcore.NewJSONEncoder(sentryAndLogzEncoderConfig)
	consoleEncoder := zapcore.NewConsoleEncoder(consoleEncoderConfig)

	// Join the outputs, encoders, and level-handling functions into
	// zapcore.Cores, then tee the four cores together.
	core := zapcore.NewTee(
		zapcore.NewCore(sentryAndLogzEncoder, errors, highPriority),
		zapcore.NewCore(consoleEncoder, consoleErrors, highPriority),
		zapcore.NewCore(sentryAndLogzEncoder, debugging, lowPriority),
		zapcore.NewCore(consoleEncoder, consoleDebugging, lowPriority),
	)

	logger = zap.New(core)
}

// Close flushes all production logging (i.e. Sentry and Logzio).
func Close() {
	// Flush buffered logging events before the program terminates.
	// Info("Flushing Sentry...")
	// FlushSentry()
	// Info("Flushing Logzio...")
	// stopAndDrainLogzio()
	logger.Sync()
}

// Info logs some info + timestamp, but does not send it to Sentry.
func Info(v ...interface{}) {
	logger.Sugar().Info(v...)
}

// Error logs an error and sends it to Sentry.
func Error(err error) {
	logger.Sugar().Error(err)
}

// Warning logs an error in red text, like Error, but doesn't send it to
// Sentry.
func Warning(err error) {
	logger.Sugar().Warn(err)
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
	// if logzioTransport != nil {
	// 	logzioTransport.send(errstr, logzioTypeError)
	// }
	// if sentryTransport != nil {
	// 	sentryTransport.send(err)
	// }
	PrintStackTrace()

	if globalCancel != nil {
		Info(err)
		globalCancel()
	} else {
		// If we're truly trying to panic, let's at least flush our logging queues
		// first so this error actually gets sent.
		FlushLogzio()
		FlushSentry()
		logger.Sugar().Panic(err)
	}
}

// Infof is identical to Info, since Info already respects printf syntax. We
// only include Infof for consistency with Errorf, Warningf, and Panicf.
func Infof(format string, v ...interface{}) {
	logger.Sugar().Infof(format, v...)
}

// Errorf is like Error, but it respects printf syntax, i.e. takes in a format
// string and arguments, for convenience.
func Errorf(format string, v ...interface{}) {
	logger.Sugar().Errorf(format, v...)
}

// Warningf is like Warning, but it respects printf syntax, i.e. takes in a format
// string and arguments, for convenience.
func Warningf(format string, v ...interface{}) {
	logger.Sugar().Warnf(format, v...)
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

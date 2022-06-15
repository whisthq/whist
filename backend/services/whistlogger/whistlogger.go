package whistlogger

import (
	"context"
	"os"
	"runtime/debug"
	"strings"

	"github.com/whisthq/whist/backend/services/utils"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

var logger *zap.Logger

func init() {
	// Define the logic for filtering messages according to their level.
	// For Logzio we want all messages, but for Sentry only the errors
	// are considered.
	onlyErrors := zap.LevelEnablerFunc(func(lvl zapcore.Level) bool {
		return lvl >= zapcore.ErrorLevel
	})
	allLevels := zap.LevelEnablerFunc(func(lvl zapcore.Level) bool {
		return true
	})

	// Define a stdout with Locking so that the logging methods are
	// safe for concurrent use.
	consoleDebugging := zapcore.Lock(os.Stdout)

	// Define configurations for each core.
	sentryEncoderConfig := NewSentryEncoderConfig()
	logzEncoderConfig := NewLogzioEncoderConfig()
	consoleEncoderConfig := zap.NewDevelopmentEncoderConfig()

	// Enable colored output on stdout
	consoleEncoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder

	// Create the new encoders for each core.
	sentryEncoder := zapcore.NewJSONEncoder(sentryEncoderConfig)
	logzEncoder := zapcore.NewJSONEncoder(logzEncoderConfig)
	consoleEncoder := zapcore.NewConsoleEncoder(consoleEncoderConfig)

	logzCore := NewLogzioCore(logzEncoder, allLevels)
	sentryCore := NewSentryCore(sentryEncoder, onlyErrors)

	// Join the outputs, encoders, and level-handling functions into
	// zapcore.Cores, then tee the four cores together.
	core := zapcore.NewTee(
		logzCore,
		sentryCore,
		zapcore.NewCore(consoleEncoder, consoleDebugging, allLevels),
	)

	// Once everything is configured, instanciate the logger.
	logger = zap.New(core)
}

// Sync is a function that flushes the queues and sends the events to the
// corresponding output. This should be called before exiting the program.
func Sync() {
	err := logger.Sync()
	if err != nil && !strings.Contains(err.Error(), "sync /dev/stdout: invalid argument") {
		Errorf("failed to drain log queues: %s", err)
	}
}

// Info constructs a log message.
func Info(v ...interface{}) {
	logger.Sugar().Info(v...)
}

// Error logs an error.
func Error(err error) {
	logger.Sugar().Error(err)
}

// Warning logs a warning message.
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
	PrintStackTrace()

	if globalCancel != nil {
		Info(err)
		globalCancel()
	} else {
		// If we're truly trying to panic, let's at least flush our logging queues
		// first so this error actually gets sent.
		logger.Sync()
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

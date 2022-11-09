package logger

import (
	"context"
	"fmt"
	"os"
	"runtime/debug"
	"strings"

	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

var logger *zap.Logger

func init() {
	// Define the logic for filtering messages according to their level.
	// For Logzio we want all messages, but for Sentry only the errors
	// are considered. In addition, we disable debug messages (such as metrics)
	// in console output to avoid polluting stdout.
	onlyErrors := zap.LevelEnablerFunc(func(lvl zapcore.Level) bool {
		return lvl >= zapcore.ErrorLevel
	})
	logsAndErrors := zap.LevelEnablerFunc(func(lvl zapcore.Level) bool {
		return lvl >= zapcore.InfoLevel
	})
	allLevels := zap.LevelEnablerFunc(func(lvl zapcore.Level) bool {
		return true
	})

	// Define a stdout with Locking so that the logging methods are
	// safe for concurrent use.
	consoleDebugging := zapcore.Lock(os.Stdout)

	// Define configurations for each core.
	sentryEncoderConfig := newSentryEncoderConfig()
	logzEncoderConfig := newLogzioEncoderConfig()
	consoleEncoderConfig := zap.NewDevelopmentEncoderConfig()

	// Enable colored output on stdout
	consoleEncoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder

	// Create the new encoders for each core.
	sentryEncoder := zapcore.NewJSONEncoder(sentryEncoderConfig)
	logzEncoder := zapcore.NewJSONEncoder(logzEncoderConfig)
	consoleEncoder := zapcore.NewConsoleEncoder(consoleEncoderConfig)

	logzCore := newLogzioCore(logzEncoder, allLevels)
	sentryCore := newSentryCore(sentryEncoder, onlyErrors)

	// Join the outputs, encoders, and level-handling functions into
	// zapcore.Cores, then tee the four cores together.
	core := zapcore.NewTee(
		logzCore,
		sentryCore,
		zapcore.NewCore(consoleEncoder, consoleDebugging, logsAndErrors),
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

// Debug constructs a debug message.
func Debug(v ...interface{}) {
	logger.Sugar().Debug(v...)
}

// Info constructs a log message.
func Info(v ...interface{}) {
	logger.Sugar().Info(v...)
}

// FastInfo constructs a log message using a faster logger. This should only be used
// for performance-sensitive code where every allocation and microsecond matter, as
// the logger only supports structured, strongly-typed fields.
func FastInfo(msg string, fields ...zapcore.Field) {
	logger.Info(msg, fields...)
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
		Error(err)
		globalCancel()
	} else {
		// If we're truly trying to panic, let's at least flush our logging queues
		// first so this error actually gets sent.
		logger.Sync()
		logger.Sugar().Panic(err)
	}
}

// Debugf is like Info, but it respects printf syntax, i.e. takes in a format
// string and arguments, for convenience.
func Debugf(format string, v ...interface{}) {
	logger.Sugar().Debugf(format, v...)
}

// Infof is like Info, but it respects printf syntax, i.e. takes in a format
// string and arguments, for convenience.
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
	Panic(globalCancel, fmt.Errorf(format, v...))
}

// Debugw constructs a debug message with additional context fields.
func Debugw(msg string, fields []interface{}) {
	logger.Sugar().Debugw(msg, fields...)
}

// Infow constructs a debug message with additional context fields.
func Infow(msg string, fields []interface{}) {
	logger.Sugar().Infow(msg, fields...)
}

// Warningw constructs a debug message with additional context fields.
func Warningw(msg string, fields []interface{}) {
	logger.Sugar().Warnw(msg, fields...)
}

// Errorw constructs a debug message with additional context fields.
func Errorw(msg string, fields []interface{}) {
	logger.Sugar().Errorw(msg, fields...)
}

// Panicw constructs a debug message with additional context fields.
func Panicw(msg string, fields []interface{}) {
	logger.Sugar().Panicw(msg, fields...)
}

// PrintStackTrace prints the stack trace, for debugging purposes.
func PrintStackTrace() {
	Info("Printing stack trace: ")
	debug.PrintStack()
}

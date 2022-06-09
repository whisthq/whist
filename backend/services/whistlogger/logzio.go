package whistlogger // import "github.com/whisthq/whist/backend/services/whistlogger"

import (
	"log"
	"os"
	"time"

	"github.com/logzio/logzio-go"
	"github.com/whisthq/whist/backend/services/utils"
	"go.uber.org/zap/zapcore"
)

type logzioCore struct {
	enabler zapcore.LevelEnabler
	encoder zapcore.Encoder
	sender  *logzio.LogzioSender // logz client
}

func NewLogzioCore(encoder zapcore.Encoder, levelEnab zapcore.LevelEnabler) zapcore.Core {
	logzioShippingToken := os.Getenv("LOGZIO_SHIPPING_TOKEN")
	sender, err := logzio.New(
		logzioShippingToken,
		logzio.SetUrl("https://listener.logz.io:8071"),
		logzio.SetDrainDuration(time.Second*3),
		logzio.SetCheckDiskSpace(false),
	)
	if err != nil {
		log.Printf("Couldn't marshal payload for logz.io. Error: %s", err)
		return nil
	}

	lc := &logzioCore{}
	lc.encoder = encoder
	lc.enabler = levelEnab
	lc.sender = sender

	return lc
}

func NewLogzioEncoderConfig() zapcore.EncoderConfig {
	return zapcore.EncoderConfig{
		TimeKey:        "timestamp",
		LevelKey:       "type",
		NameKey:        "logger",
		CallerKey:      "caller",
		FunctionKey:    zapcore.OmitKey,
		MessageKey:     "message",
		StacktraceKey:  "stacktrace",
		LineEnding:     zapcore.DefaultLineEnding,
		EncodeLevel:    zapcore.LowercaseLevelEncoder,
		EncodeTime:     zapcore.EpochTimeEncoder,
		EncodeDuration: zapcore.SecondsDurationEncoder,
		EncodeCaller:   zapcore.ShortCallerEncoder,
	}
}

func (lc *logzioCore) Enabled(level zapcore.Level) bool {
	return lc.enabler.Enabled(level)
}

func (lc *logzioCore) With(fields []zapcore.Field) zapcore.Core {
	core := &logzioCore{
		enabler: lc.enabler,
		encoder: lc.encoder.Clone(),
		sender:  lc.sender,
	}

	for i := range fields {
		fields[i].AddTo(core.encoder)
	}

	return core
}

func (lc *logzioCore) Check(ent zapcore.Entry, ce *zapcore.CheckedEntry) *zapcore.CheckedEntry {
	if lc.Enabled(ent.Level) {
		return ce.AddCore(ent, lc)
	}
	return ce
}

func (lc *logzioCore) Write(ent zapcore.Entry, fields []zapcore.Field) error {
	if usingProdLogging() {
		return nil
	}

	buf, err := lc.encoder.EncodeEntry(ent, fields)
	if err != nil {
		return err
	}
	// Write to logzio
	err = lc.sender.Send(buf.Bytes())
	buf.Free()
	if err != nil {
		return utils.MakeError("Couldn't marshal payload for logz.io. Error: %s", err)
	}
	if ent.Level > zapcore.ErrorLevel {
		// Since we may be crashing the program, sync the output.
		lc.Sync()
	}
	return nil
}

func (lc *logzioCore) Sync() error {
	//Flush logzio
	return lc.sender.Sync()
}

package ecsagent // import "github.com/fractal/fractal/ecs-host-service/ecsagent"

import (
	fractallogger "github.com/fractal/ecs-agent/agent/fractallogger"

	"github.com/cihub/seelog"
)

// This file contains the machinery needed for us to bridge the ecs agent's
// `seelog`-based logging infrastructure to our logger.

type trivialSeeLogReceiver struct{}

// ReceiveMessage ignores trace and debug messages.
func (r *trivialSeeLogReceiver) ReceiveMessage(message string, level seelog.LogLevel, context seelog.LogContextInterface) error {
	switch level {
	case seelog.TraceLvl:
		return nil
	case seelog.DebugLvl:
		return nil
	case seelog.InfoLvl:
		fractallogger.Infof("INFO from %s: %s\n", context.Func(), message)
	case seelog.WarnLvl:
		fractallogger.Infof("WARN from %s: %s\n", context.Func(), message)
	case seelog.ErrorLvl:
		fallthrough
	case seelog.CriticalLvl:
		fractallogger.Errorf("ERROR from %s: %s\n", context.Func(), message)
	}

	return nil
}

func (r *trivialSeeLogReceiver) AfterParse(_ seelog.CustomReceiverInitArgs) error { return nil }

func (r *trivialSeeLogReceiver) Flush() {}

func (r *trivialSeeLogReceiver) Close() error { return nil }

package fractallogger

import (
	_ "github.com/logzio/logzio-go"
)

// Variable containing our logz.io shipping token --- filled in by linker if built on CI
var logzioShippingToken string

func initLogz() error {
	if logzioShippingToken == "" {
		return MakeError("logzioShippingToken is uninitialized!")
	}

	return nil
}

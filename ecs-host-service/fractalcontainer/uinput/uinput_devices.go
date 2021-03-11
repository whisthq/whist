package uinput // import "github.com/fractal/fractal/ecs-host-service/fractalcontainer/uinput"

import (
	uinput "github.com/fractal/uinput-go"
)

type UinputDevices struct {
	absmouse uinput.TouchPad
	relmouse uinput.Mouse
	keyboard uinput.Keyboard
}

package uinput // import "github.com/fractal/fractal/ecs-host-service/mandelbox/uinputdevices/uinput"

import (
	"fmt"
	"io"
	"os"
)

// A TouchPad is an input device that uses absolute axis events, meaning that you can specify
// the exact position the cursor should move to. Therefore, it is necessary to define the size
// of the rectangle in which the cursor may move upon creation of the device.
type TouchPad interface {
	// MoveTo will move the cursor to the specified position on the screen
	MoveTo(x int32, y int32) error

	// MouseButtonClick will issue a mouse button click.
	MouseButtonClick(buttonCode int) error

	// MouseButtonPress will simulate the press of a mouse button. Note that the button will not be released until
	// MouseButtonRelease is invoked.
	MouseButtonPress(buttonCode int) error

	// MouseButtonRelease will simulate the release of a mouse button.
	MouseButtonRelease(buttonCode int) error

	// TouchDown will simulate a single touch to a virtual touch device. Use TouchUp to end the touch gesture.
	TouchDown() error

	// TouchUp will end or ,more precisely, unset the touch event issued by TouchDown
	TouchUp() error

	Name() []byte

	DeviceFile() *os.File

	io.Closer
}

type vTouchPad struct {
	name       []byte
	deviceFile *os.File
}

func (vTouch vTouchPad) Name() []byte {
	return vTouch.name
}

func (vTouch vTouchPad) DeviceFile() *os.File {
	return vTouch.deviceFile
}

// CreateTouchPad will create a new touch pad device. note that you will need to define the x and y axis boundaries
// (min and max) within which the cursor maybe moved around.
func CreateTouchPad(path string, name []byte, minX int32, maxX int32, minY int32, maxY int32) (TouchPad, error) {
	err := validateDevicePath(path)
	if err != nil {
		return nil, err
	}
	err = validateUinputName(name)
	if err != nil {
		return nil, err
	}

	fd, err := createTouchPad(path, name, minX, maxX, minY, maxY)
	if err != nil {
		return nil, err
	}

	return vTouchPad{name: name, deviceFile: fd}, nil
}

func (vTouch vTouchPad) MoveTo(x int32, y int32) error {
	return sendAbsEvent(vTouch.deviceFile, x, y)
}

// MouseButtonClick will issue a MouseButtonClick
func (vTouch vTouchPad) MouseButtonClick(buttonCode int) error {
	err := sendBtnEvent(vTouch.deviceFile, []int{buttonCode}, btnStatePressed)
	if err != nil {
		return fmt.Errorf("Failed to issue the 0x%x mouse button event: %v", buttonCode, err)
	}

	return sendBtnEvent(vTouch.deviceFile, []int{buttonCode}, btnStateReleased)
}

// MouseButtonPress will simulate the press of a mouse button. Note that the button will not be released until
// MouseButtonRelease is invoked.
func (vTouch vTouchPad) MouseButtonPress(buttonCode int) error {
	return sendBtnEvent(vTouch.deviceFile, []int{buttonCode}, btnStatePressed)
}

// MouseButtonRelease will simulate the release of a mouse button.
func (vTouch vTouchPad) MouseButtonRelease(buttonCode int) error {
	return sendBtnEvent(vTouch.deviceFile, []int{buttonCode}, btnStateReleased)
}

func (vTouch vTouchPad) TouchDown() error {
	return sendBtnEvent(vTouch.deviceFile, []int{evBtnTouch, evBtnToolFinger}, btnStatePressed)
}

func (vTouch vTouchPad) TouchUp() error {
	return sendBtnEvent(vTouch.deviceFile, []int{evBtnTouch, evBtnToolFinger}, btnStateReleased)
}

func (vTouch vTouchPad) Close() error {
	return closeDevice(vTouch.deviceFile)
}

func createTouchPad(path string, name []byte, minX int32, maxX int32, minY int32, maxY int32) (fd *os.File, err error) {
	deviceFile, err := createDeviceFile(path)
	if err != nil {
		return nil, fmt.Errorf("could not create absolute axis input device: %v", err)
	}

	err = registerDevice(deviceFile, uintptr(evKey))
	if err != nil {
		deviceFile.Close()
		return nil, fmt.Errorf("failed to register key device: %v", err)
	}
	// register button events
	for _, event := range []int{evBtnTouch, evBtnToolPen, evBtnStylus} {
		err = ioctl(deviceFile, uiSetKeyBit, uintptr(event))
		if err != nil {
			deviceFile.Close()
			return nil, fmt.Errorf("failed to register button event %v: %v", event, err)
		}
	}

	err = registerDevice(deviceFile, uintptr(evAbs))
	if err != nil {
		deviceFile.Close()
		return nil, fmt.Errorf("failed to register absolute axis input device: %v", err)
	}

	// register x and y axis events
	for _, event := range []int{absX, absY} {
		err = ioctl(deviceFile, uiSetAbsBit, uintptr(event))
		if err != nil {
			deviceFile.Close()
			return nil, fmt.Errorf("failed to register absolute axis event %v: %v", event, err)
		}
	}

	err = ioctl(deviceFile, uiSetPropBit, inputPropPointer)

	var absMin [absSize]int32
	absMin[absX] = minX
	absMin[absY] = minY

	var absMax [absSize]int32
	absMax[absX] = maxX
	absMax[absY] = maxY

	return createUsbDevice(deviceFile,
		uinputUserDev{
			Name: toUinputName(name),
			ID: inputID{
				Bustype: busUsb,
				Vendor:  0xf4c1,
				Product: 0x1124,
				Version: 1},
			Absmin: absMin,
			Absmax: absMax})
}

func sendAbsEvent(deviceFile *os.File, xPos int32, yPos int32) error {
	var ev [2]inputEvent
	ev[0].Type = evAbs
	ev[0].Code = absX
	ev[0].Value = xPos

	// Various tests (using evtest) have shown that positioning on x=0;y=0 doesn't trigger any event and will not move
	// the cursor as expected. Setting at least one of the coordinates to -1 will however have the desired effect of
	// moving the cursor to the upper left corner. Interestingly, the same is true for equivalent code in C, which rules
	// out issues related to Go's data type representation or the like. This will need to be investigated further...
	if xPos == 0 && yPos == 0 {
		yPos--
	}

	ev[1].Type = evAbs
	ev[1].Code = absY
	ev[1].Value = yPos

	for _, iev := range ev {
		buf, err := inputEventToBuffer(iev)
		if err != nil {
			return fmt.Errorf("writing abs event failed: %v", err)
		}

		_, err = deviceFile.Write(buf)
		if err != nil {
			return fmt.Errorf("failed to write abs event to device file: %v", err)
		}
	}

	return syncEvents(deviceFile)
}

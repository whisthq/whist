package uinput // import "github.com/fractal/fractal/host-service/mandelbox/uinputdevices/uinput"

import (
	"fmt"
	"io"
	"os"
	"syscall"
)

// A Mouse is a device that will trigger an absolute change event.
// For details see: https://www.kernel.org/doc/Documentation/input/event-codes.txt
type Mouse interface {
	// MoveLeft will move the mouse cursor left by the given number of pixel.
	MoveLeft(pixel int32) error

	// MoveRight will move the mouse cursor right by the given number of pixel.
	MoveRight(pixel int32) error

	// MoveUp will move the mouse cursor up by the given number of pixel.
	MoveUp(pixel int32) error

	// MoveDown will move the mouse cursor down by the given number of pixel.
	MoveDown(pixel int32) error

	// Move will perform a move of the mouse pointer along the x and y axes relative to the current position as requested.
	// Note that the upper left corner is (0, 0), so positive x and y means moving right (x) and down (y), whereas negative
	// values will cause a move towards the upper left corner.
	Move(x, y int32) error

	// MouseButtonClick will issue a mouse button click.
	MouseButtonClick(buttonCode int) error

	// MouseButtonPress will simulate the press of a mouse button. Note that the button will not be released until
	// MouseButtonRelease is invoked.
	MouseButtonPress(buttonCode int) error

	// MouseButtonRelease will simulate the release of a mouse button.
	MouseButtonRelease(buttonCode int) error

	// Wheel will simulate a wheel movement.
	Wheel(horizontal bool, delta int32) error

	Name() []byte

	DeviceFile() *os.File

	io.Closer
}

type vMouse struct {
	name       []byte
	deviceFile *os.File
}

func (vRel vMouse) Name() []byte {
	return vRel.name
}

func (vRel vMouse) DeviceFile() *os.File {
	return vRel.deviceFile
}

// CreateMouse will create a new mouse input device. A mouse is a device that allows relative input.
// Relative input means that all changes to the x and y coordinates of the mouse pointer will be
func CreateMouse(path string, name []byte) (Mouse, error) {
	err := validateDevicePath(path)
	if err != nil {
		return nil, err
	}
	err = validateUinputName(name)
	if err != nil {
		return nil, err
	}

	fd, err := createMouse(path, name)
	if err != nil {
		return nil, err
	}

	return vMouse{name: name, deviceFile: fd}, nil
}

// MoveLeft will move the cursor left by the number of pixel specified.
func (vRel vMouse) MoveLeft(pixel int32) error {
	if err := assertNotNegative(pixel); err != nil {
		return err
	}
	return sendRelEvent(vRel.deviceFile, relX, -pixel)
}

// MoveRight will move the cursor right by the number of pixel specified.
func (vRel vMouse) MoveRight(pixel int32) error {
	if err := assertNotNegative(pixel); err != nil {
		return err
	}
	return sendRelEvent(vRel.deviceFile, relX, pixel)
}

// MoveUp will move the cursor up by the number of pixel specified.
func (vRel vMouse) MoveUp(pixel int32) error {
	if err := assertNotNegative(pixel); err != nil {
		return err
	}
	return sendRelEvent(vRel.deviceFile, relY, -pixel)
}

// MoveDown will move the cursor down by the number of pixel specified.
func (vRel vMouse) MoveDown(pixel int32) error {
	if err := assertNotNegative(pixel); err != nil {
		return err
	}
	return sendRelEvent(vRel.deviceFile, relY, pixel)
}

// Move will perform a move of the mouse pointer along the x and y axes relative to the current position as requested.
// Note that the upper left corner is (0, 0), so positive x and y means moving right (x) and down (y), whereas negative
// values will cause a move towards the upper left corner.
func (vRel vMouse) Move(x, y int32) error {
	if err := sendRelEvent(vRel.deviceFile, relX, x); err != nil {
		return fmt.Errorf("Failed to move pointer along x axis: %v", err)
	}
	if err := sendRelEvent(vRel.deviceFile, relY, y); err != nil {
		return fmt.Errorf("Failed to move pointer along y axis: %v", err)
	}
	return nil
}

// MouseButtonClick will issue a MouseButtonClick
func (vRel vMouse) MouseButtonClick(buttonCode int) error {
	err := sendBtnEvent(vRel.deviceFile, []int{buttonCode}, btnStatePressed)
	if err != nil {
		return fmt.Errorf("Failed to issue the 0x%x mouse button event: %v", buttonCode, err)
	}

	return sendBtnEvent(vRel.deviceFile, []int{buttonCode}, btnStateReleased)
}

// MouseButtonPress will simulate the press of a mouse button. Note that the button will not be released until
// MouseButtonRelease is invoked.
func (vRel vMouse) MouseButtonPress(buttonCode int) error {
	return sendBtnEvent(vRel.deviceFile, []int{buttonCode}, btnStatePressed)
}

// MouseButtonRelease will simulate the release of a mouse button.
func (vRel vMouse) MouseButtonRelease(buttonCode int) error {
	return sendBtnEvent(vRel.deviceFile, []int{buttonCode}, btnStateReleased)
}

// Wheel will simulate a wheel movement.
func (vRel vMouse) Wheel(horizontal bool, delta int32) error {
	w := relWheel
	if horizontal {
		w = relHWheel
	}
	return sendRelEvent(vRel.deviceFile, uint16(w), delta)
}

// Close closes the device and releases the device.
func (vRel vMouse) Close() error {
	return closeDevice(vRel.deviceFile)
}

func createMouse(path string, name []byte) (fd *os.File, err error) {
	deviceFile, err := createDeviceFile(path)
	if err != nil {
		return nil, fmt.Errorf("could not create relative axis input device: %v", err)
	}

	err = registerDevice(deviceFile, uintptr(evKey))
	if err != nil {
		deviceFile.Close()
		return nil, fmt.Errorf("failed to register key device: %v", err)
	}

	// register button events (in order to enable left, right, middle, button 4, and button 5 click)
	for _, event := range []int{evBtnLeft, evBtnRight, evBtnMiddle, evBtnSide, evBtnExtra} {
		err = ioctl(deviceFile, uiSetKeyBit, uintptr(event))
		if err != nil {
			deviceFile.Close()
			return nil, fmt.Errorf("failed to register click event %v: %v", event, err)
		}
	}

	err = registerDevice(deviceFile, uintptr(evRel))
	if err != nil {
		deviceFile.Close()
		return nil, fmt.Errorf("failed to register relative axis input device: %v", err)
	}

	// register relative events
	for _, event := range []int{relX, relY, relWheel, relHWheel, relWheelHiRes, relHWheelHiRes} {
		err = ioctl(deviceFile, uiSetRelBit, uintptr(event))
		if err != nil {
			deviceFile.Close()
			return nil, fmt.Errorf("failed to register relative event %v: %v", event, err)
		}
	}

	return createUsbDevice(deviceFile,
		uinputUserDev{
			Name: toUinputName(name),
			ID: inputID{
				Bustype: busUsb,
				Vendor:  0xf4c1,
				Product: 0x1123,
				Version: 1}})
}

func sendRelEvent(deviceFile *os.File, eventCode uint16, pixel int32) error {
	iev := inputEvent{
		Time:  syscall.Timeval{Sec: 0, Usec: 0},
		Type:  evRel,
		Code:  eventCode,
		Value: pixel}

	buf, err := inputEventToBuffer(iev)
	if err != nil {
		return fmt.Errorf("writing abs event failed: %v", err)
	}

	_, err = deviceFile.Write(buf)
	if err != nil {
		return fmt.Errorf("failed to write rel event to device file: %v", err)
	}

	return syncEvents(deviceFile)
}

func assertNotNegative(val int32) error {
	if val < 0 {
		return fmt.Errorf("%v is out of range. Expected a positive or zero value", val)
	}
	return nil
}

package uinputdevices // import "github.com/fractal/fractal/ecs-host-service/fractalcontainer/uinputdevices"

import (
	"bytes"
	"context"
	"net"
	"os"
	"path/filepath"
	"strings"
	"syscall"
	"unsafe"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"

	uinput "github.com/fractal/uinput-go"

	dockercontainer "github.com/docker/docker/api/types/container"
)

type UinputDevices struct {
	absmouse uinput.TouchPad
	relmouse uinput.Mouse
	keyboard uinput.Keyboard
}

// Note: we don't need a lock in this package, since the operating system will
// take care of that for us.

// Allocate() allocates uinput devices atomically (i.e. either one succeeds, or
// none of them do).
func Allocate() (devices *UinputDevices, mappings []dockercontainer.DeviceMapping, reterr error) {
	var absmouse uinput.TouchPad
	var absmousePath string
	var relmouse uinput.Mouse
	var relmousePath string
	var keyboard uinput.Keyboard
	var keyboardPath string
	var err error

	absmouse, err = uinput.CreateTouchPad("/dev/uinput", []byte("Fractal Virtual Absolute Input"), 0, 0xFFF, 0, 0xFFF)
	if err != nil {
		reterr = logger.MakeError("Could not create virtual absolute input: %s", reterr)
		return
	}
	defer func() {
		if reterr != nil {
			absmouse.Close()
		}
	}()

	absmousePath, err = getDeviceFilePath(absmouse.DeviceFile())
	if err != nil {
		reterr = logger.MakeError("Failed to get device path for virtual absolute input: %s", err)
		return
	}

	relmouse, err = uinput.CreateMouse("/dev/uinput", []byte("Fractal Virtual Relative Input"))
	if err != nil {
		reterr = logger.MakeError("Could not create virtual relative input: %s", err)
		return
	}
	defer func() {
		if reterr != nil {
			relmouse.Close()
		}
	}()

	relmousePath, err = getDeviceFilePath(relmouse.DeviceFile())
	if err != nil {
		reterr = logger.MakeError("Failed to get device path for virtual relative input: %s", err)
		return
	}

	keyboard, err = uinput.CreateKeyboard("/dev/uinput", []byte("Fractal Virtual Keyboard"))
	if err != nil {
		reterr = logger.MakeError("Could not create virtual keyboard: %s", err)
		return
	}
	defer func() {
		if reterr != nil {
			keyboard.Close()
		}
	}()

	keyboardPath, err = getDeviceFilePath(keyboard.DeviceFile())
	if err != nil {
		reterr = logger.MakeError("Failed to get device path for virtual keyboard: %s", err)
		return
	}

	devices = &UinputDevices{absmouse, relmouse, keyboard}

	mappings = []dockercontainer.DeviceMapping{
		{
			PathOnHost:        absmousePath,
			PathInContainer:   absmousePath,
			CgroupPermissions: "rwm", // read, write, mknod (the default)
		},
		{
			PathOnHost:        relmousePath,
			PathInContainer:   relmousePath,
			CgroupPermissions: "rwm", // read, write, mknod (the default)
		},
		{
			PathOnHost:        keyboardPath,
			PathInContainer:   keyboardPath,
			CgroupPermissions: "rwm", // read, write, mknod (the default)
		},
	}

	return
}

func SendDeviceFDsOverSocket(baseCtx context.Context, devices *UinputDevices, socketPath string) error {
	// Create our own context so we can safely cancel it.
	ctx, cancel := context.WithCancel(baseCtx)
	defer cancel()

	dir, err := filepath.Abs(filepath.Dir(socketPath))
	if err != nil {
		return logger.MakeError("Unable to calculate parent directory of given socketPath %s. Error: %s", socketPath, err)
	}

	os.MkdirAll(dir, 0777)
	os.RemoveAll(socketPath)

	var lc net.ListenConfig
	listener, err := lc.Listen(ctx, "unix", socketPath)
	if err != nil {
		return logger.MakeError("Could not create unix socket at %s: %s", socketPath, err)
	}
	// Instead of running `defer listener.Close()` we ran `defer cancel()` above,
	// which will cause the following goroutine to always close `listener`.
	// Normally, listener.Accept() blocks until the protocol connects. If the
	// context is closed, though, we want to unblock.
	// TODO: SYNC.WAITGROUP
	go func() {
		<-ctx.Done()
		listener.Close()
	}()

	logger.Infof("Successfully created unix socket at: %s", socketPath)

	// listener.Accept() blocks until the protocol connects
	client, err := listener.Accept()
	if err != nil {
		logger.MakeError("Could not connect to client over unix socket at %s: %s", socketPath, err)
	}
	defer client.Close()

	connf, err := client.(*net.UnixConn).File()
	if err != nil {
		logger.Errorf("Could not get file corresponding to client connection for unix socket at %s: %s", socketPath, err)
	}
	defer connf.Close()

	connfd := int(connf.Fd())
	fds := [3]int{
		int(devices.absmouse.DeviceFile().Fd()),
		int(devices.relmouse.DeviceFile().Fd()),
		int(devices.keyboard.DeviceFile().Fd()),
	}
	rights := syscall.UnixRights(fds[:]...)
	err = syscall.Sendmsg(connfd, nil, rights, nil, 0)
	if err != nil {
		return logger.MakeError("Error sending uinput file descriptors over socket: %s", err)
	}

	logger.Infof("Sent uinput file descriptors to socket at %s", socketPath)
	return nil
}

// getDeviceFilePath returns the file path (e.g. /dev/input/event3) given a
// file descriptor corresponding to a virtual device created with uinput
func getDeviceFilePath(fd *os.File) (string, error) {
	const maxlen uintptr = 32
	bsysname := make([]byte, maxlen)
	_, _, errno := syscall.Syscall(syscall.SYS_IOCTL, uintptr(fd.Fd()), linuxUIGetSysName(maxlen), uintptr(unsafe.Pointer(&bsysname[0])))
	if errno != 0 {
		return "", logger.MakeError("ioctl to get sysname failed with errno %d", errno)
	}
	sysname := string(bytes.Trim(bsysname, "\x00"))
	syspath := logger.Sprintf("/sys/devices/virtual/input/%s", sysname)
	sysdir, err := os.Open(syspath)
	if err != nil {
		return "", logger.MakeError("could not open directory %s: %s", syspath, err)
	}
	names, err := sysdir.Readdirnames(0)
	if err != nil {
		return "", logger.MakeError("Readdirnames for directory %s failed: %s", syspath, err)
	}
	for _, name := range names {
		if strings.HasPrefix(name, "event") {
			return "/dev/input/" + name, nil
		}
	}
	return "", logger.MakeError("did not find file in %s with prefix 'event'", syspath)
}

// TODO: document this
func linuxUIGetSysName(len uintptr) uintptr {
	// from ioctl.h and uinput.h
	const (
		iocDirshift  = 30
		iocSizeshift = 16
		iocTypeshift = 8
		iocNrshift   = 0
		iocRead      = 2
		uiIoctlBase  = 85 // 'U'
	)

	linuxIoc := func(dir uintptr, requestType uintptr, nr uintptr, size uintptr) uintptr {
		return (dir << iocDirshift) + (requestType << iocTypeshift) + (nr << iocNrshift) + (size << iocSizeshift)
	}

	return linuxIoc(iocRead, uiIoctlBase, 44, len)
}

func (u *UinputDevices) Close() {
	u.absmouse.Close()
	u.relmouse.Close()
	u.keyboard.Close()
}

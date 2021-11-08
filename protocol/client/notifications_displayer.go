package main

import (
	"C"

	"github.com/gen2brain/beeep"
)

//export DisplayNotification
func DisplayNotification(title *C.char, message *C.char) {
	beeep.Notify(C.GoString(title), C.GoString(message), "../information.png")
}

func main() {}

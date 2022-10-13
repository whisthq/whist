import { Socket } from "socket.io-client"

const initPreferencesInitHandler = (socket: Socket) => {
	socket.on("init-preferences", ([isDarkMode]: [boolean]) => {
		chrome.runtime.setDarkMode(isDarkMode, () => socket.emit("darkmode-initialized"))
	})
}

const initDarkModeChangeHandler = (socket: Socket) => {
	socket.on("darkmode-changed", ([isDarkMode]: [boolean]) => {
		chrome.runtime.setDarkMode(isDarkMode)
	})
}

export {
	initPreferencesInitHandler,
	initDarkModeChangeHandler,
}
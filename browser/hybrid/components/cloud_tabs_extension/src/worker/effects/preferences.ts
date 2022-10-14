import { withLatestFrom } from "rxjs/operators"
import { Socket } from "socket.io-client"

import { darkModeChanged } from "@app/worker/events/preferences"
import { socket, socketConnected } from "@app/worker/events/socketio"

import { whistState } from "@app/worker/utils/state"

socketConnected.subscribe(async (socket: Socket) => {
    // Set keyboard repeat rate
    // TODO: actually get the repeat rate
    socket.emit("keyboard-repeat-rate-changed", 20, 200)

    // Set dark mode
    const usingDarkMode = new Promise((resolve) => {
        ;(chrome as any).braveTheme.getBraveThemeType((themeType: string) => {
          resolve(themeType === "Dark")
        })
    })

    socket.emit("init-preferences", await usingDarkMode)
})

darkModeChanged
    .pipe(withLatestFrom(socket))
    .subscribe(([isDarkMode, socket]: [boolean, Socket]) => {
        socket.emit("darkmode-changed", isDarkMode)
    })
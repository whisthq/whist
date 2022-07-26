import { Socket } from "socket.io-client"
import omit from "lodash.omit"

const initAddCookieListener = (socket: Socket) => {
  socket.on("add-cookie", (cookies: any[]) => {
    console.log("adding cookie", cookies)
    let cookie = omit(cookies[0], ["hostOnly", "session"]) as chrome.cookies.SetDetails
    chrome.cookies.set(cookie)
  })
}

const initRemoveCookieListener = (socket: Socket) => {
  socket.on("remove-cookie", (cookies: any[]) => {
    const cookie = cookies[0]
    const url = cookie.domain.startsWith(".")
      ? `https://${cookie.domain.slice(1)}`
      : `https://${cookie.domain}`
    chrome.cookies.remove({
      name: cookie.name,
      url,
    })
  })
}

export { initAddCookieListener, initRemoveCookieListener }

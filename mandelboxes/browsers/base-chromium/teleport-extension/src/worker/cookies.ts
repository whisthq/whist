import { Socket } from "socket.io-client"

const initAddCookieListener = (socket: Socket) => {
  socket.on("add-cookie", (cookies: any[]) => {
    chrome.cookies.set(cookies[0])
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

import { Socket } from "socket.io-client"
import omit from "lodash.omit"

const initAddCookieListener = (socket: Socket) => {
  socket.on("add-cookie", (cookies: any[]) => {
    let cookie = cookies[0]
    const url = cookie.domain.startsWith(".")
      ? `https://${cookie.domain.slice(1)}${cookie.path}`
      : `https://${cookie.domain}${cookie.path}`

    const details = {
        ...omit(cookie, ["hostOnly", "session"]),
        url,
        ...cookie.hostOnly ? {} : {domain: cookie.domain},
        ...!cookie.session && {expirationDate: cookie.expirationDate}
    } as chrome.cookies.SetDetails

    console.log("Setting", details)

    chrome.cookies.set(details)
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

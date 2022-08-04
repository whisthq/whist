import { Socket } from "socket.io-client"
import find from "lodash.find"
import isEqual from "lodash.isequal"

// v3 global variables don't persist, but we only need to store cookies we've already added
// for a few seconds so we don't fall into a loop of adding a cookie and then thinking that
// the cookie we've added was added by the user and not us
let alreadyAddedCookies: chrome.cookies.Cookie[] = []

const initAddCookieListener = (socket: Socket) => {
  socket.on("server-add-cookie", (cookies: any[]) => {
    let cookie = cookies[0]

    const url = cookie.domain.startsWith(".")
      ? `https://${cookie.domain.slice(1)}${cookie.path}`
      : `https://${cookie.domain}${cookie.path}`

    // See https://developer.chrome.com/docs/extensions/reference/cookies/#method-set
    // for why we construct the cookie details this way
    const details = {
      ...(!cookie.hostOnly && { domain: cookie.domain }),
      ...(!cookie.session && { expirationDate: cookie.expirationDate }),
      httpOnly: cookie.httpOnly,
      name: cookie.name,
      path: cookie.path,
      sameSite: cookie.sameSite,
      secure: cookie.secure,
      storeId: cookie.storeId,
      value: cookie.value,
      url,
    } as chrome.cookies.SetDetails

    alreadyAddedCookies.push(cookie)
    chrome.cookies.set(details)
  })
}

const initSyncCookieListener = (socket: Socket) => {
  socket.on("sync-cookies", (cookies: chrome.cookies.Cookie[]) => {
    console.log("Sync cookies received", cookies)
    cookies.forEach((cookie) => {
      console.log("Syncing", cookie)
      const url = cookie.domain?.startsWith(".")
        ? `https://${cookie.domain?.slice(1)}`
        : `https://${cookie.domain}`

      const details = {
        ...(!cookie.hostOnly && { domain: cookie.domain }),
        ...(!cookie.session && { expirationDate: cookie.expirationDate }),
        httpOnly: cookie.httpOnly,
        name: cookie.name,
        path: cookie.path,
        sameSite: cookie.sameSite,
        secure: cookie.secure,
        storeId: cookie.storeId,
        value: cookie.value,
        url,
      } as chrome.cookies.SetDetails

      alreadyAddedCookies.push(cookie)
      chrome.cookies.set(details)
    })
  })
}

const initRemoveCookieListener = (socket: Socket) => {
  socket.on("server-remove-cookie", (cookies: any[]) => {
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

const initCookieAddedListener = (socket: Socket) => {
  chrome.cookies.onChanged.addListener(
    (details: {
      cause: string
      cookie: chrome.cookies.Cookie
      removed: boolean
    }) => {
      if (
        find(alreadyAddedCookies, (c) => isEqual(c, details.cookie)) !==
        undefined
      )
        return

      if (!details.removed && details.cause === "explicit") {
        socket.emit("client-add-cookie", details.cookie)
      }
    }
  )
}

export {
  initAddCookieListener,
  initRemoveCookieListener,
  initCookieAddedListener,
  initSyncCookieListener,
}

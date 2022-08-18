import { Socket } from "socket.io-client"
import find from "lodash.find"
import isEqual from "lodash.isequal"

import { cookieToDetails } from "@app/utils/cookies"

// v3 global variables don't persist, but we only need to store cookies we've already added
// for a few seconds so we don't fall into a loop of adding a cookie and then thinking that
// the cookie we've added was added by the user and not us
let alreadyAddedCookies: chrome.cookies.Cookie[] = []

const initAddCookieListener = (socket: Socket) => {
  socket.on("server-add-cookie", async ([cookie]: [chrome.cookies.Cookie]) => {
    alreadyAddedCookies.push(cookie)
    chrome.cookies.set(cookieToDetails(cookie))
  })
}

const initRemoveCookieListener = (socket: Socket) => {
  socket.on("server-remove-cookie", ([cookie]: [chrome.cookies.Cookie]) => {
    const url = cookie.domain.startsWith(".")
      ? `https://${cookie.domain.slice(1)}`
      : `https://${cookie.domain}`
    chrome.cookies.remove({
      name: cookie.name,
      url,
    })
  })
}

const initCookieSyncHandler = (socket: Socket) => {
  socket.on("sync-cookies", async (cookies: chrome.cookies.Cookie[]) => {
    // while (cookies.length > 0) {
    //   const cookie = cookies.shift()
    //   if (cookie !== undefined) {
    //     alreadyAddedCookies.push(cookie)
    //     await chrome.cookies.set(cookieToDetails(cookie))
    //   }
    // }

    await Promise.all(cookies.map((cookie) => chrome.cookies.set(cookieToDetails(cookie))))
    socket.emit("cookie-sync-complete")
    chrome.storage.sync.set({ cookiesSynced: true })
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

      chrome.storage.sync.get(["cookiesSynced"], ({ cookiesSynced }) => {
        if (cookiesSynced && !details.removed && details.cause === "explicit") {
          socket.emit("client-add-cookie", details.cookie)
        }
      })
    }
  )
}

const initCookieRemovedListener = (socket: Socket) => {
  chrome.cookies.onChanged.addListener(
    (details: {
      cause: string
      cookie: chrome.cookies.Cookie
      removed: boolean
    }) => {
      chrome.storage.sync.get(["cookiesSynced"], ({ cookiesSynced }) => {
        if (
          cookiesSynced &&
          details.removed &&
          ["expired", "expired_overwrite", "evicted"].includes(details.cause)
        ) {
          socket.emit("client-remove-cookie", details.cookie)
        }
      })
    }
  )
}

export {
  initAddCookieListener,
  initRemoveCookieListener,
  initCookieAddedListener,
  initCookieRemovedListener,
  initCookieSyncHandler,
}

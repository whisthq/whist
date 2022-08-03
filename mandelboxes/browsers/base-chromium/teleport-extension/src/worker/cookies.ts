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
    console.log("Setting", cookie)
    chrome.cookies.set(details)
  })

  socket.on("waiting-cookies", (info: any) => {
    console.log("Got waiting cookies", info)
    while(info.length > 0) {
      const cookie = info.shift()
      if (cookie === undefined || cookie.cookie.name.startsWith("fractal")) 
	      return
      const url = cookie.domain.startsWith(".")
        ? `https://${cookie.domain.slice(1)}`
        : `https://${cookie.domain}`
        
      if (cookie.removed) {
        console.log("removing", cookie.cookie)
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
        console.log("Setting", cookie)
        chrome.cookies.set(details)
      } else {
        console.log("adding", cookie.cookie)
        chrome.cookies.remove({
          name: cookie.name,
          url,
        })
      }
    }
  })
}

const initRemoveCookieListener = (socket: Socket) => {
  socket.on("server-remove-cookie", (cookies: any[]) => {
    const cookie = cookies[0]
    const url = cookie.domain.startsWith(".")
      ? `https://${cookie.domain.slice(1)}`
      : `https://${cookie.domain}`
    console.log("Removing", cookie)
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
      ) {
	console.log("Flagged, already added", details.cookie)
        return
      }

      if (!details.removed && details.cause === "explicit") {
	console.log("Client should add", details.cookie)
        socket.emit("client-add-cookie", details.cookie)
      }
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
      if (
        details.removed &&
        ["expired", "expired_overwrite", "evicted"].includes(details.cause)
      ) {
	console.log("Client should remove", details.cookie)
        socket.emit("client-remove-cookie", details.cookie)
      }
    }
  )
}

export {
  initAddCookieListener,
  initRemoveCookieListener,
  initCookieAddedListener,
  initCookieRemovedListener,
}

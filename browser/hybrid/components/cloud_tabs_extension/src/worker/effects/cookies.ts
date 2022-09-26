import { withLatestFrom } from "rxjs/operators"
import { Socket } from "socket.io-client"

import {
  clientCookieAdded,
  clientCookieRemoved,
  serverCookieAdded,
} from "@app/worker/events/cookies"
import { socket, socketConnected } from "@app/worker/events/socketio"

import { whistState } from "@app/worker/utils/state"

// If a cookie is added in a local tab, add it to a cloud tab
clientCookieAdded
  .pipe(withLatestFrom(socket))
  .subscribe(([cookie, socket]: [chrome.cookies.Cookie, Socket]) => {
    socket.emit("server-add-cookie", cookie)
  })

// If a cookie is removed from a local tab, remove it from the cloud tab
clientCookieRemoved
  .pipe(withLatestFrom(socket))
  .subscribe(([cookie, socket]: [chrome.cookies.Cookie, Socket]) => {
    socket.emit("server-remove-cookie", cookie)
  })

socketConnected.subscribe((socket: Socket) => {
  chrome.cookies.getAll({}, (cookies) => {
    socket.emit("sync-cookies", ...cookies)
  })
})

// If the server adds a cookie, also add it on the client side
// We add this cookie to an "alreadyAddedCookies" list to make sure
// we don't fall into an infinite loop of adding cookies and then
// sending them back to the server to be added
serverCookieAdded.subscribe(([cookie]: [chrome.cookies.Cookie]) => {
  if (cookie.domain.includes("google")) return

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

  whistState.alreadyAddedCookies.push(cookie)
  void chrome.cookies.set(details)
})

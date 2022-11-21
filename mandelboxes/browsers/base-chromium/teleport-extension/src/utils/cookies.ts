const cookieToDetails = (cookie: chrome.cookies.Cookie) => {
  const url = cookie.domain.startsWith(".")
    ? `https://${cookie.domain.slice(1)}${cookie.path}`
    : `https://${cookie.domain}${cookie.path}`

  return {
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
}

export { cookieToDetails }

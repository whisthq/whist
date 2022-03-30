const createNotification = (document: any, text: string) => {
  // For security, some websites block injected HTML, so we use the TrustedHTML
  // API to bypass this.
  // For more info: https://developer.mozilla.org/en-US/docs/Web/API/TrustedHTML
  const escapeHTMLPolicy = (window as any).trustedTypes.createPolicy("policy", {
    createHTML: (s: string) => s.replace(/\>/g, "<"),
  })

  let notification = document.createElement("p")

  notification.style.width = "400px"
  notification.style.background = "rgba(0, 0, 0, 0.8)"
  notification.style.color = "white"
  notification.style.position = "fixed"
  notification.style.top = "10px"
  notification.style.right = "10px"
  notification.style.borderRadius = "8px"
  notification.style.padding = "14px 20px"
  notification.style.zIndex = "99999999"
  notification.style.fontSize = "12px"
  notification.style.color = "rgb(209 213 219)"
  notification.innerHTML = escapeHTMLPolicy.createHTML(text)
  ;(document.body || document.documentElement).appendChild(notification)
}

export { createNotification }

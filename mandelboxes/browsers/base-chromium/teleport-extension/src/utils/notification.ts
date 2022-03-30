const createNotification = (document: any, text: string) => {
  // Create the notification HTMLElement
  let div = document.createElement("p")

  div.style.width = "400px"
  div.style.background = "rgba(0, 0, 0, 0.8)"
  div.style.color = "white"
  div.style.position = "fixed"
  div.style.top = "10px"
  div.style.right = "10px"
  div.style.borderRadius = "8px"
  div.style.padding = "14px 20px"
  div.style.zIndex = "99999999"
  div.style.fontSize = "12px"
  div.style.color = "rgb(209 213 219)"

  // For security, some websites block injected HTML, so we use the TrustedHTML
  // API to bypass this.
  // For more info: https://developer.mozilla.org/en-US/docs/Web/API/TrustedHTML
  const escapeHTMLPolicy = (window as any).trustedTypes.createPolicy("policy", {
    createHTML: (s: string) => s.replace(/\>/g, "<"),
  })
  div.innerHTML = escapeHTMLPolicy.createHTML(text)

  // Inject the HTMLElement into the DOM
  ;(document.body || document.documentElement).appendChild(div)
}

export { createNotification }

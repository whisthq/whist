const fadeOut = (element: HTMLElement) => {
  var op = 1 // initial opacity
  var timer = setInterval(function () {
    if (op <= 0.1) {
      clearInterval(timer)
      element.style.display = "none"
    }
    element.style.opacity = op.toString()
    element.style.filter = "alpha(opacity=" + op * 100 + ")"
    op -= op * 0.1
  }, 25)
}

const fadeIn = (element: HTMLElement) => {
  var op = 0.1 // initial opacity
  element.style.display = "block"
  var timer = setInterval(function () {
    if (op >= 1) {
      clearInterval(timer)
    }
    element.style.opacity = op.toString()
    element.style.filter = "alpha(opacity=" + op * 100 + ")"
    op += op * 0.1
  }, 25)
}

const createNotification = (document: Document, text: string) => {
  // Create the notification HTMLElement
  let element = document.createElement("p")

  element.style.width = "400px"
  element.style.background = "#111827"
  element.style.position = "fixed"
  element.style.bottom = "10px"
  element.style.left = "10px"
  element.style.borderRadius = "8px"
  element.style.padding = "14px 20px"
  element.style.zIndex = "99999999"
  element.style.fontSize = "12px"
  element.style.color = "#d1d5db"
  element.style.opacity = "0"
  element.style.fontFamily = "Helvetica"
  element.style.boxShadow =
    "0 4px 6px -1px rgb(0 0 0 / 0.1), 0 2px 4px -2px rgb(0 0 0 / 0.1)"
  element.style.letterSpacing = "0.05em"
  element.style.lineHeight = "1rem"

  // For security, some websites block injected HTML, so we use the TrustedHTML
  // API to bypass this.
  // For more info: https://developer.mozilla.org/en-US/docs/Web/API/TrustedHTML
  const escapeHTMLPolicy = (window as any).trustedTypes.createPolicy("policy", {
    createHTML: (s: string) => s.replace(/\>/g, "<"),
  })
  element.innerHTML = escapeHTMLPolicy.createHTML(text)

  // Inject the HTMLElement into the DOM
  ;(document.body || document.documentElement).appendChild(element)

  fadeIn(element)
  setTimeout(() => fadeOut(element), 10000)
}

const drawLeftArrow = (document: Document) => {
  // Create the notification HTMLElement
  let element = document.createElement("p")

  element.style.width = "100px"
  element.style.borderRadius = "0px 50px 50px 0px"
  element.style.background = "rgba(0, 0, 0, 0.7)"
  element.style.position = "fixed"
  element.style.top = "50%"
  element.style.left = "0px"
  element.style.padding = "10px"
  element.style.zIndex = "99999999"

  // For security, some websites block injected HTML, so we use the TrustedHTML
  // API to bypass this.
  // For more info: https://developer.mozilla.org/en-US/docs/Web/API/TrustedHTML
  const escapeHTMLPolicy = (window as any).trustedTypes.createPolicy("policy", {
    createHTML: (s: string) => s.replace(/\>/g, "<"),
  })
  // element.innerHTML =

  // Inject the HTMLElement into the DOM
  ;(document.body || document.documentElement).appendChild(element)

  fadeIn(element)
  setTimeout(() => fadeOut(element), 10000)
}

export { createNotification, drawLeftArrow }

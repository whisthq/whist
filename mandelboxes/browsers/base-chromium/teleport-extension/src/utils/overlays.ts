// For security, some websites block injected HTML, so we use the TrustedHTML
// API to bypass this.
// For more info: https://developer.mozilla.org/en-US/docs/Web/API/TrustedHTML
const escapeHTMLPolicy = (window as any).trustedTypes.createPolicy("policy", {
  createHTML: (s: string) => s,
})

const fadeOut = (element: HTMLElement, speed = 1) => {
  var op = 1 // initial opacity
  var timer = setInterval(function () {
    if (op <= 0.1) {
      clearInterval(timer)
      element.style.display = "none"
    }
    element.style.opacity = op.toString()
    element.style.filter = "alpha(opacity=" + op * 100 + ")"
    op -= op * 0.1
  }, 25 / speed)
}

const fadeIn = (element: HTMLElement, speed = 1) => {
  var op = 0.1 // initial opacity
  element.style.display = "block"
  var timer = setInterval(function () {
    if (op >= 1) {
      clearInterval(timer)
    }
    element.style.opacity = op.toString()
    element.style.filter = "alpha(opacity=" + op * 100 + ")"
    op += op * 0.1
  }, 25 / speed)
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
  element.style.fontFamily = "Helvetica"
  element.style.boxShadow =
    "0 4px 6px -1px rgb(0 0 0 / 0.1), 0 2px 4px -2px rgb(0 0 0 / 0.1)"
  element.style.letterSpacing = "0.05em"
  element.style.lineHeight = "1rem"

  element.innerHTML = escapeHTMLPolicy.createHTML(text)

  // Inject the HTMLElement into the DOM
  ;(document.body || document.documentElement).appendChild(element)

  fadeIn(element)
  setTimeout(() => fadeOut(element), 10000)
}

const drawArrow = (document: Document, direction: string) => {
  // Create the notification HTMLElement
  let element = document.createElement("div")
  let inner = document.createElement("div")

  element.style.width = "70px"
  element.style.height = "70px"
  element.style.borderRadius = "100%"
  element.style.background = "rgba(0, 146, 255, 0.2)"
  element.style.position = "fixed"
  element.style.top = "46%"
  element.style.zIndex = "99999999"
  element.style.opacity = "1"
  element.style.color = "white"
  element.style.fontFamily = "Helvetica"
  element.style.display = "flex"
  element.style.flexDirection = "row"
  element.style.flexWrap = "wrap"
  element.style.justifyContent = "center"
  element.style.alignItems = "center"

  inner.style.width = "50%"
  inner.style.height = "50%"
  inner.style.background = "rgba(0, 146, 255)"
  inner.style.borderRadius = "100%"
  inner.style.display = "flex"
  inner.style.flexDirection = "row"
  inner.style.flexWrap = "wrap"
  inner.style.justifyContent = "center"
  inner.style.alignItems = "center"

  if (direction === "back") {
    element.style.left = "-70px"
    inner.innerHTML += escapeHTMLPolicy.createHTML(
      '<svg style="position: relative; top: 20px; color: white; left: 10x; width: 20px;" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2.5"><path stroke-linecap="round" stroke-linejoin="round" d="M10 19l-7-7m0 0l7-7m-7 7h18" /></svg>'
    )
  } else {
    element.style.right = "-70px"
    inner.innerHTML += escapeHTMLPolicy.createHTML(
      '<svg style="position: relative; top: 20px; color: white; left: 10x; width: 20px;" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2.5"><path stroke-linecap="round" stroke-linejoin="round" d="M14 5l7 7m0 0l-7 7m7-7H3" /></svg>'
    )
  }

  // Inject the HTMLElement into the DOM
  element.appendChild(inner)
  ;(document.body || document.documentElement).appendChild(element)

  return {
    update: (progress: number) => {
      const offset = `${-70 + progress * 1.5}px`
      const fill = `${Math.max(50, progress)}%`

      if (direction === "back") {
        element.style.left = offset
      } else {
        element.style.right = offset
      }

      inner.style.width = fill
      inner.style.height = fill
    },
    remove: () => element.remove(),
  }
}

export { createNotification, drawArrow }

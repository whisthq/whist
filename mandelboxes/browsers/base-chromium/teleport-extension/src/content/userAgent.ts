const initUserAgentSpoofer = () => {
  var s = document.createElement("script")
  s.src = chrome.runtime.getURL("js/userAgent.js")
  s.onload = () => {
    s.remove()
  }
  ;(document.head || document.documentElement).appendChild(s)
}

export { initUserAgentSpoofer }

const injectScriptIntoDOM = (document: any, filePath: string) => {
  var s = document.createElement("script")
  s.src = chrome.runtime.getURL(filePath)
  s.onload = () => {
    s.remove()
  }
  ;(document.head || document.documentElement).appendChild(s)
}

export { injectScriptIntoDOM }

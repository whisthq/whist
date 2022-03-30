import { injectScriptIntoDOM } from "@app/utils/dom"

const initUserAgentSpoofer = () => {
  injectScriptIntoDOM(document, "js/userAgent.js")
}

export { initUserAgentSpoofer }

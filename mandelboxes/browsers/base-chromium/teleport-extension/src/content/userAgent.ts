import { injectResourceIntoDOM } from "@app/utils/dom"

const initUserAgentSpoofer = () => {
  injectResourceIntoDOM(document, "js/userAgent.js")
}

export { initUserAgentSpoofer }

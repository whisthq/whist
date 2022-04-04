import { injectResourceIntoDOM } from "@app/utils/resources"

const initUserAgentSpoofer = () => {
  injectResourceIntoDOM(document, "js/userAgent.js")
}

export { initUserAgentSpoofer }

import { injectResourceIntoDOM } from "@app/utils/dom"

const initFeatureWarnings = () => {
  injectResourceIntoDOM(document, "js/notification.js")
}

export { initFeatureWarnings }

import { injectResourceIntoDOM } from "@app/utils/dom"

const initFeatureWarnings = () => {
  injectResourceIntoDOM(document, "js/warning.js")
}

export { initFeatureWarnings }

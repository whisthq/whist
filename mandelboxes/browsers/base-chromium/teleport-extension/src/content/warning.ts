import { injectScriptIntoDOM } from "@app/utils/dom"

const initFeatureWarnings = () => {
  injectScriptIntoDOM(document, "js/warning.js")
}

export { initFeatureWarnings }

import { injectResourceIntoDOM } from "@app/utils/dom"

const initOverscroll = () => {
  injectResourceIntoDOM(document, "js/overscroll.js")
}

export { initOverscroll }

import { injectResourceIntoDOM } from "@app/utils/dom"

const initSwipeGestures = () => {
  injectResourceIntoDOM(document, "js/overscroll.js")
}

export { initSwipeGestures }

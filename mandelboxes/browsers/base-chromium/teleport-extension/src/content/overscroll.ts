import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"

const lastN = <T>(limit: number, array: Array<T>) => {
  if (array.length > limit) array.splice(0, array.length - limit)
  const fluent = {
    add: (value: T) => {
      if (array.length >= limit) array.splice(0, array.length - limit + 1)
      array.push(value)
      return fluent
    },
    getAll: () => array,
  }
  return fluent
}

let previousOffset = 0
let throttled = false
let previousYDeltas = lastN<number>(10, [])
let previousXDeltas = lastN<number>(10, [])

const navigateOnGesture = (e: WheelEvent) => {
  if (
    e.offsetX - previousOffset === 0 &&
    e.deltaX > 100 &&
    !throttled &&
    !previousYDeltas.getAll().some((delta) => Math.abs(delta) > 10)
  ) {
    throttled = true
    // chrome.runtime.sendMessage(<ContentScriptMessage>{
    //   type: ContentScriptMessageType.HISTORY_GO_FORWARD,
    // })

    setTimeout(() => {
      throttled = false
    }, 2000)

    console.log("SCROLLING")
  }

  if (
    e.offsetX - previousOffset === 0 &&
    e.deltaX < -100 &&
    !throttled &&
    !previousYDeltas.getAll().some((delta) => Math.abs(delta) > 10)
  ) {
    throttled = true
    // chrome.runtime.sendMessage(<ContentScriptMessage>{
    //   type: ContentScriptMessageType.HISTORY_GO_BACK,
    // })

    setTimeout(() => {
      throttled = false
    }, 2000)

    console.log("SCROLLING")
  }

  previousOffset = e.offsetX
  previousYDeltas.add(e.deltaY)
  previousXDeltas.add(e.deltaX)
}

const initOverscroll = () => {
  window.addEventListener("wheel", navigateOnGesture)
}

export { initOverscroll }

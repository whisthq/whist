const clamp = (v: number, min: number, max: number): number =>
  Math.max(Math.min(v, max), min)

// Clamp scaleFactor between 1 and 10
const clampScale = (scaleFactor: number): number => clamp(scaleFactor, 1, 10)

const zoomSpeed = (0.03 / 5) * 0.7

const globalState = {
  scaleFactor: 1,
  translation: {
    x: 0,
    y: 0,
  },
}

// When in quirks mode, scroll fields do not work on the <html> element, but instead the body.
const scrollBoxElement =
  document.compatMode === "BackCompat"
    ? document.body
    : document.documentElement

// Only update cached translation after scroll events that we know about,
// because otherwise integer scrollLeft/scrollTop will cause visual glitches with
// fractional scrolls.
const updateTranslationFromScroll = () => {
  globalState.translation.x = -scrollBoxElement.scrollLeft
  globalState.translation.y = -scrollBoxElement.scrollTop
}

const applyScaleTransform = (scale: number) => {
  const style = document.documentElement.style

  style.setProperty("transform", `scale(${scale})`, "important")
  style.setProperty("transform-origin", "0 0", "important")

  // Hack to restore normal behavior that is upset by the transform
  style.position = "relative"
  style.height = "100%"

  // Weird performance hack, apparently. Maybe the browser is batching changes?
  style.transitionProperty = "transform, left, top"
  style.transitionDuration = "0s"
}

const getOffscreenDimensions = (): { x: number; y: number } => {
  return {
    x: scrollBoxElement.scrollWidth - scrollBoxElement.clientWidth,
    y: scrollBoxElement.scrollHeight - scrollBoxElement.clientHeight,
  }
}

const handleWheelEvent = (e: WheelEvent) => {
  if (!e.ctrlKey || e.defaultPrevented) return

  const scrollBoxRelativeCoordinates = {
    x: e.clientX - scrollBoxElement.offsetLeft,
    y: e.clientY - scrollBoxElement.offsetTop,
  }

  e.preventDefault()
  e.stopPropagation()

  const oldScaleFactor = globalState.scaleFactor
  globalState.scaleFactor = clampScale(oldScaleFactor - e.deltaY * zoomSpeed)

  if (oldScaleFactor === globalState.scaleFactor) return

  applyScaleTransform(globalState.scaleFactor)

  const tx =
    (globalState.translation.x - scrollBoxRelativeCoordinates.x) *
      (globalState.scaleFactor / oldScaleFactor) +
    scrollBoxRelativeCoordinates.x
  const ty =
    (globalState.translation.y - scrollBoxRelativeCoordinates.y) *
      (globalState.scaleFactor / oldScaleFactor) +
    scrollBoxRelativeCoordinates.y

  // apply new xy-translation
  const offscreenDimensions = getOffscreenDimensions()
  scrollBoxElement.scrollLeft = clamp(-tx, 0, offscreenDimensions.x)
  scrollBoxElement.scrollTop = clamp(-ty, 0, offscreenDimensions.y)
}

const handleKeyDownEvent = (e: KeyboardEvent) => {
  // Cmd+0 to reset zoom to default
  if (!e.ctrlKey || e.key !== "0") return

  // Reset state
  globalState.scaleFactor = 1

  const oldScrollLeft = scrollBoxElement.scrollLeft
  const oldScrollTop = scrollBoxElement.scrollTop
  const oldOffscreenDimensions = getOffscreenDimensions()
  applyScaleTransform(globalState.scaleFactor)
  const newOffscreenDimensions = getOffscreenDimensions()

  scrollBoxElement.scrollLeft =
    oldScrollLeft * (newOffscreenDimensions.x / oldOffscreenDimensions.x)
  scrollBoxElement.scrollTop =
    oldScrollTop * (newOffscreenDimensions.y / oldOffscreenDimensions.y)

  updateTranslationFromScroll()
}

const handleScrollEvent = () => {
  updateTranslationFromScroll()
}

const toggleCustomPinchToZoom = () => {
  // If the document contains a canvas it probably has its own zoom handler, so we don't add ours
  if (document.getElementsByTagName("canvas").length === 0) {
    window.addEventListener("keydown", handleKeyDownEvent)

    // { passive: false } indicates that this event handler may call preventDefault
    window.addEventListener("scroll", handleScrollEvent, { passive: false })
    document.documentElement.addEventListener("wheel", handleWheelEvent, {
      passive: false,
    })
  } else {
    window.removeEventListener("scroll", handleScrollEvent)
    window.removeEventListener("keydown", handleKeyDownEvent)
    document.documentElement.removeEventListener("wheel", handleWheelEvent)
  }
}

const initPinchToZoom = () => {
  const observer = new MutationObserver(toggleCustomPinchToZoom)
  observer.observe(document, {
    attributes: false,
    childList: true,
    subtree: true,
  })
}

export { initPinchToZoom }

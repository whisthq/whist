const fixAppearance = () => {
  chrome.runtime.getPlatformInfo((info) => {
    if (info.os === "mac") {
      setTimeout(() => {
        document.body.style.width = `${document.body.clientWidth + 1}px`
      }, 250)
    }
  })
}

export { fixAppearance }

// Add "Open Link in Cloud Tab" functionality to right-click menus
chrome.contextMenus.create({
  contexts: ["link", "image", "video"],
  title: "Open Link in Cloud Tab",
  onclick: (info) => {
    if (info.linkUrl !== undefined)
      void chrome.tabs.create({ url: `cloud:${info.linkUrl}` })
  },
})

// Add "Open Image in Cloud Tab" functionality to right-click menus
chrome.contextMenus.create({
  contexts: ["image"],
  title: "Open Image in Cloud Tab",
  onclick: (info) => {
    if (info.srcUrl !== undefined)
      void chrome.tabs.create({ url: `cloud:${info.srcUrl}` })
  },
})

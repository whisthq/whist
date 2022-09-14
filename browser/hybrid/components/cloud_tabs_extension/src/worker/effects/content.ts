import { showUpdatePage, showLoginPage } from "@app/worker/events/content"

showUpdatePage.subscribe(() => {
  chrome.tabs.create({
    url: "whist://settings/help",
  })
})

showLoginPage.subscribe(() => {
  ;(chrome as any).whist.setWhistIsLoggedIn(false, () => {
    chrome.tabs.create({
      url: "whist://welcome",
    })
  })
})

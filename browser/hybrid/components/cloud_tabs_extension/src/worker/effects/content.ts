import { showUpdatePage, showLoginPage } from "@app/worker/events/content"

showUpdatePage.subscribe(() => {
  void chrome.tabs.create({
    url: "whist://settings/help",
  })
})

showLoginPage.subscribe(() => {
  ;(chrome as any).whist.setWhistIsLoggedIn(false, () => {
    void chrome.tabs.create({
      url: "whist://welcome",
    })
  })
})

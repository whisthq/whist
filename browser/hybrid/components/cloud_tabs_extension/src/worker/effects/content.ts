import { showUpdatePage, showLoginPage } from "@app/worker/events/content"

showUpdatePage.subscribe(() => {
  void chrome.tabs.create({
    url: "whist://settings/help",
  })
})

showLoginPage.subscribe(() => {
  void chrome.tabs.create({
    url: "whist://welcome",
  })
})

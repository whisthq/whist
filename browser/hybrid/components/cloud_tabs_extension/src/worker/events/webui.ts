import { fromEventPattern } from "rxjs"
import { filter, map, share } from "rxjs/operators"

const webuiNavigate = fromEventPattern(
  (handler: any) => (chrome as any).whist.onMessage.addListener(handler),
  (handler: any) => (chrome as any).whist.onMessage.removeListener(handler),
  (message: any) => message
).pipe(
  map((message: string) => JSON.parse(message)),
  filter((message: any) => message.type === "NAVIGATION"),
  map((message: any) => message.value),
  share()
)

const webuiOpenSupport = fromEventPattern(
  (handler: any) => (chrome as any).whist.onMessage.addListener(handler),
  (handler: any) => (chrome as any).whist.onMessage.removeListener(handler),
  (message: any) => message
).pipe(
  map((message: string) => JSON.parse(message)),
  filter((message: any) => message.type === "OPEN_SUPPORT"),
  share()
)

const webuiRefresh = fromEventPattern(
  (handler: any) => (chrome as any).whist.onMessage.addListener(handler),
  (handler: any) => (chrome as any).whist.onMessage.removeListener(handler),
  (message: any) => message
).pipe(
  map((message: string) => JSON.parse(message)),
  filter((message: any) => message.type === "REFRESH"),
  map((message: any) => message.value),
  share()
)

const webuiMandelboxNeeded = fromEventPattern(
  (handler: any) => (chrome as any).whist.onMessage.addListener(handler),
  (handler: any) => (chrome as any).whist.onMessage.removeListener(handler),
  (message: any) => message
).pipe(
  map((message: string) => JSON.parse(message)),
  filter((message: any) => message.type === "MANDELBOX_NEEDED"),
  share()
)

chrome.runtime.onMessage.addListener((message: any) => {
  if (message.type === "SHOW_UPDATE_PAGE") {
    chrome.tabs.create({
      url: "whist://settings/help",
    })
  }
})

export { webuiNavigate, webuiOpenSupport, webuiRefresh, webuiMandelboxNeeded }

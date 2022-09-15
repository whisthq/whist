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

const welcomePageOpened = fromEventPattern(
  (handler: any) => (chrome as any).whist.onMessage.addListener(handler),
  (handler: any) => (chrome as any).whist.onMessage.removeListener(handler),
  (message: any) => message
).pipe(
  map((message: string) => JSON.parse(message)),
  filter((message: any) => message.type === "WELCOME_PAGE_OPENED"),
  share()
)

const webuiDragEntered = fromEventPattern(
  (handler: any) => (chrome as any).whist.onMessage.addListener(handler),
  (handler: any) => (chrome as any).whist.onMessage.removeListener(handler),
  (message: any) => message
).pipe(
  map((message: string) => JSON.parse(message)),
  filter((message: any) => message.type === "DRAG_ENTERED"),
  map((message: any) => message.value),
  share()
)

export {
  webuiNavigate,
  webuiOpenSupport,
  webuiRefresh,
  webuiMandelboxNeeded,
  welcomePageOpened,
  webuiDragEntered
}

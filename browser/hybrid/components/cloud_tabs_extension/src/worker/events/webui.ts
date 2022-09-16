import { fromEventPattern, from } from "rxjs"
import { filter, map, share, switchMap } from "rxjs/operators"
import { getTab } from "@app/worker/utils/tabs"

const webUINavigate = fromEventPattern(
  (handler: any) => (chrome as any).whist.onMessage.addListener(handler),
  (handler: any) => (chrome as any).whist.onMessage.removeListener(handler),
  (message: any) => message
).pipe(
  map((message: string) => JSON.parse(message)),
  filter((message: any) => message.type === "NAVIGATION"),
  map((message: any) => message.value),
  share()
)

const webUIOpenSupport = fromEventPattern(
  (handler: any) => (chrome as any).whist.onMessage.addListener(handler),
  (handler: any) => (chrome as any).whist.onMessage.removeListener(handler),
  (message: any) => message
).pipe(
  map((message: string) => JSON.parse(message)),
  filter((message: any) => message.type === "OPEN_SUPPORT"),
  share()
)

const webUIRefresh = fromEventPattern(
  (handler: any) => (chrome as any).whist.onMessage.addListener(handler),
  (handler: any) => (chrome as any).whist.onMessage.removeListener(handler),
  (message: any) => message
).pipe(
  map((message: string) => JSON.parse(message)),
  filter((message: any) => message.type === "REFRESH"),
  map((message: any) => message.value),
  share()
)

const webUIMandelboxNeeded = fromEventPattern(
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

const webUIMouseEntered = fromEventPattern(
  (handler: any) => (chrome as any).whist.onMessage.addListener(handler),
  (handler: any) => (chrome as any).whist.onMessage.removeListener(handler),
  (message: any) => message
).pipe(
  map((message: string) => JSON.parse(message)),
  filter((message: any) => message.type === "MOUSE_ENTERED"),
  switchMap((message: any) => from(getTab(message.value.id))),
  share()
)

export {
  webUINavigate,
  webUIOpenSupport,
  webUIRefresh,
  webUIMandelboxNeeded,
  welcomePageOpened,
  webUIMouseEntered
}

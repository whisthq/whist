import { fromEventPattern, from, of, zip } from "rxjs"
import { filter, map, share, switchMap } from "rxjs/operators"
import { getTab } from "@app/worker/utils/tabs"

const webUiNavigate = fromEventPattern(
  (handler: any) => (chrome as any).whist.onMessage.addListener(handler),
  (handler: any) => (chrome as any).whist.onMessage.removeListener(handler),
  (message: any) => message
).pipe(
  map((message: string) => JSON.parse(message)),
  filter((message: any) => message.type === "NAVIGATION"),
  map((message: any) => message.value),
  share()
)

const webUiOpenSupport = fromEventPattern(
  (handler: any) => (chrome as any).whist.onMessage.addListener(handler),
  (handler: any) => (chrome as any).whist.onMessage.removeListener(handler),
  (message: any) => message
).pipe(
  map((message: string) => JSON.parse(message)),
  filter((message: any) => message.type === "OPEN_SUPPORT"),
  share()
)

const webUiRefresh = fromEventPattern(
  (handler: any) => (chrome as any).whist.onMessage.addListener(handler),
  (handler: any) => (chrome as any).whist.onMessage.removeListener(handler),
  (message: any) => message
).pipe(
  map((message: string) => JSON.parse(message)),
  filter((message: any) => message.type === "REFRESH"),
  map((message: any) => message.value),
  share()
)

const webUiMandelboxNeeded = fromEventPattern(
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

const webUiMouseEntered = fromEventPattern(
  (handler: any) => (chrome as any).whist.onMessage.addListener(handler),
  (handler: any) => (chrome as any).whist.onMessage.removeListener(handler),
  (message: any) => message
).pipe(
  map((message: string) => JSON.parse(message)),
  filter((message: any) => message.type === "MOUSE_ENTERED"),
  switchMap((message: any) => from(getTab(message.value.id))),
  share()
)

const webUisAreFrozen = fromEventPattern(
  (handler: any) => (chrome as any).whist.onMessage.addListener(handler),
  (handler: any) => (chrome as any).whist.onMessage.removeListener(handler),
  (message: any) => message
).pipe(
  map((message: string) => JSON.parse(message)),
  filter((message: any) => message.type === "WEB_UIS_FROZEN"),
  switchMap((message: any) =>
    zip([
      from(getTab(message.value.newActiveTabId)),
      of(message.value.spotlightId),
    ])
  ),
  share()
)

export {
  webUiNavigate,
  webUiOpenSupport,
  webUiRefresh,
  webUiMandelboxNeeded,
  welcomePageOpened,
  webUiMouseEntered,
  webUisAreFrozen,
}

import { fromEventPattern } from "rxjs"
import { filter, share } from "rxjs/operators"

const showUpdatePage = fromEventPattern(
  (handler: any) => (chrome as any).runtime.onMessage.addListener(handler),
  (handler: any) => (chrome as any).runtime.onMessage.removeListener(handler),
  (message: any) => message
).pipe(
  filter((message: any) => message.type === "SHOW_UPDATE_PAGE"),
  share()
)

const showLoginPage = fromEventPattern(
  (handler: any) => (chrome as any).runtime.onMessage.addListener(handler),
  (handler: any) => (chrome as any).runtime.onMessage.removeListener(handler),
  (message: any) => message
).pipe(
  filter((message: any) => message.type === "SHOW_LOGIN_PAGE"),
  share()
)

export { showUpdatePage, showLoginPage }

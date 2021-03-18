import { createHashHistory } from "history"

export const browserHistory = createHashHistory()
export const goTo = (url: string) => {
    browserHistory.push(url)
}

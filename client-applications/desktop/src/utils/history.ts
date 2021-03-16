import { createBrowserHistory } from "history"

export const browserHistory = createBrowserHistory()
export const goTo = (url: string) => {
    browserHistory.push(url)
}

import { store } from "store/store"

export const getAdminWebserver = (webservers?: any) => {
    const state = store.getState()

    const webserver = state.MainReducer.admin
        ? state.MainReducer.admin.webserver_url
        : "dev"

    if (!webservers || !(webserver in webservers)) {
        return webserver
    } else {
        return webservers[webserver]
    }
}

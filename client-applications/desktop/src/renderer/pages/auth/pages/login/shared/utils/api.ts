import {get, post} from "@app/utils/api"

export const emailLogin = async (username: string, password: string) =>
    post({ endpoint: "/account/login", body: { username, password } })
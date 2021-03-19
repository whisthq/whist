import { post } from "@app/utils/api"

export const emailSignup = async (
    username: string,
    password: string,
    name: string,
    feedback: string
) =>
    post({
        endpoint: "/account/register",
        body: { username, password, name, feedback },
    })

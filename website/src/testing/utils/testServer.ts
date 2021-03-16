import { rest } from "msw"
import { config } from "shared/constants/config"
/*
    Handles all mock api requests using msw, mock
    requests here will override api calls in redux saga files 
*/
export const handlers = [
    // mock login request
    rest.post(config.url.WEBSERVER_URL + "/account/login", (req, res, ctx) => {
        // Persist user's authentication in the session
        return res(
            ctx.json({
                access_token: "accessToken",
                can_login: true,
                is_user: true,
                name: null,
                encrypted_config_token:
                    "f279a50d85c6f514aa6f634f0345cf85f913279da804f41438bf7967c666db527b8b5df9bc07333699541d4be4bdfa8eba99e2ee699d7d82f1c5b51bd119fe698c80b0bedef32f1a3e9e99",
                refresh_token: "refreshToken",
                verification_token: "verificationToken",
                verified: true,
            })
        )
    }),

    // mock signup request
    rest.post(
        config.url.WEBSERVER_URL + "/account/register",
        (req, res, ctx) => {
            return res(
                ctx.json({
                    access_token: "accessToken",
                    refresh_token: "refreshToken",
                    status: 200,
                    verification_token: "verificationToken",
                })
            )
        }
    ),

    // mock verify request
    rest.post(config.url.WEBSERVER_URL + "/account/verify", (req, res, ctx) => {
        return res(
            ctx.json({
                verified: true,
                status: 200,
            })
        )
    }),
    // mock email sent
    rest.post(config.url.WEBSERVER_URL + "/mail", (req, res, ctx) => {
        return res(
            ctx.json({
                verified: true,
                status: 200,
            })
        )
    }),
    rest.post(config.url.WEBSERVER_URL + "/account/update", (req, res, ctx) => {
        return res(
            ctx.json({
                verified: true,
            })
        )
    }),
    rest.get(config.url.WEBSERVER_URL + "/token/validate", (req, res, ctx) => {
        return res(
            ctx.json({
                status: 200,
                user: {
                    user_id: "test@test.co",
                },
            })
        )
    }),
]

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
                encrypted_config_token: "",
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

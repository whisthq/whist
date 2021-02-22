/* eslint-disable @typescript-eslint/no-unused-vars */
import React from "react"

import { waitFor } from "@testing-library/dom"
import { msWait } from "testing/utils/utils"
import { customRender } from "testing/utils/customRender"
import {
    invalidUser,
    startingAuthFlow,
    validUserCanLogin,
} from "testing/utils/testState"
import { chainAssign } from "testing/utils/testHelpers"
import Auth from "pages/auth/auth"

describe("Callback", () => {
    // state for loading auth tests
    const state = (mode: string, isValid: boolean = false) =>
        chainAssign([
            {
                AuthReducer: {
                    user: isValid ? validUserCanLogin : invalidUser, // they are not logged in or whatever
                    authFlow: startingAuthFlow(mode),
                },
            },
        ])
    // we can set default to true since !stripeInfo.plan || ...
    // renders the authentication component
    const render = (
        mode: string,
        testLocation?: any,
        isValid?: boolean,
        testSignup?: boolean,
        emailToken?: string
    ) => {
        const tree = customRender(
            // might show type error but should be ok (redux provides)
            <Auth
                mode={mode}
                testLocation={testLocation}
                testSignup={testSignup}
                emailToken={emailToken}
            />,
            state(mode, isValid)
        )
        return { original: tree }
    }

    describe("redirects correctly", () => {
        test("if you need to go to the auth callback", async () => {
            const location = {
                hash: "",
                pathname: "/auth/loginToken=loginToken",
                state: undefined,
            }

            const { original } = render("Log in", location, true)

            msWait(2000)

            await waitFor(async () => {
                expect(original.history.location.pathname).toBe("/callback")
            })
        })
    })
})

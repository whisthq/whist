/* eslint-disable @typescript-eslint/no-unused-vars */
import React from "react"
// import {} from "jest"

import { screen, cleanup } from "@testing-library/react"
import {
    removeNondeterminism,
    customSnapshot,
    removeRiskyProps,
} from "testing/utils/utils"
import { customRender } from "testing/utils/customRender"
import {
    invalidUser,
    validStripeInfo,
    invalidStripeInfo,
    startingAuthFlow,
    validUserCanLogin,
} from "testing/utils/testState"
import { chainAssign } from "testing/utils/testHelpers"
import Profile from "pages/dashboard/settings/profile/profile"
import { PROFILE_IDS } from "testing/utils/testIDs"

describe("<Profile />", () => {
    describe("renders correctly", () => {
        const state = (validUser: boolean, validStripe: boolean) =>
            chainAssign([
                {
                    AuthReducer: {
                        user: validUser ? validUserCanLogin : invalidUser,
                        authFlow: startingAuthFlow("Log in"),
                    },
                },
                {
                    DashboardReducer: {
                        stripeInfo: validStripe
                            ? validStripeInfo
                            : invalidStripeInfo,
                    },
                },
            ])

        const render = (validUser: boolean, validStripe: boolean) =>
            removeNondeterminism(
                customRender(<Profile />, state(validUser, validStripe))
            )

        const apply = (str: string) => removeRiskyProps(str)

        const matchSnapshot = async (tree: any, str: string) =>
            await customSnapshot(
                tree,
                `./src/pages/profile/__tests__/__snapshots__/custom-${str}`,
                apply
            )

        // const getSignoutButton = () => screen.getByTestId(PROFILE_IDS.OUT)
        const getNameForm = () => screen.getByTestId(PROFILE_IDS.NAME)
        const getPasswordForm = () => screen.getByTestId(PROFILE_IDS.PASSWORD)

        test("valid user and payment enabled", async () => {
            const tree = render(true, true)
            matchSnapshot(tree, "valid")

            getPasswordForm()
            getNameForm()
            // getSignoutButton()
        })
        // cleanup()
    })

    describe("allows you to fill out", () => {
        test.skip("name form", async () => {
            throw new Error("implement please")
        })

        test.skip("password form", async () => {
            throw new Error("implement please")
        })

        test.skip("plan form", async () => {
            throw new Error("implement please")
        })

        test.skip("payment form", async () => {
            throw new Error("implement please")
        })
    })
})

/* eslint-disable @typescript-eslint/no-unused-vars */
import React from "react"
// import {} from "jest"

import { screen } from "@testing-library/react"
import { removeNondeterminism } from "testing/utils/utils"
import { customRender } from "testing/utils/customRender"
import { CANCEL_IDS } from "testing/utils/testIDs"
import Cancel from "pages/dashboard/settings/payment/cancel/cancel"
import {
    invalidUser,
    validUserCanLogin,
    validStripeInfo,
    invalidStripeInfo,
} from "testing/utils/testState"
import { chainAssign } from "testing/utils/testHelpers"

describe("<Cancel />", () => {
    describe("renders correctly", () => {
        const state = (user: boolean, stripe: boolean) =>
            chainAssign([
                {
                    AuthReducer: {
                        user: user ? validUserCanLogin : invalidUser,
                    },
                },
                {
                    DashboardReducer: {
                        stripeInfo: stripe
                            ? validStripeInfo
                            : invalidStripeInfo,
                    },
                },
            ])

        // we can set default to true since !stripeInfo.plan || ...
        const render = (user: boolean, stripe: boolean) =>
            removeNondeterminism(
                customRender(
                    <Cancel paymentEnabled={true} />,
                    state(user, stripe)
                )
            )

        const getBackButton = () => screen.getByTestId(CANCEL_IDS.BACK)
        const getPlanInfo = () => screen.getByTestId(CANCEL_IDS.INFO)
        const getWarning = () => screen.getByTestId(CANCEL_IDS.WARN)
        const getSubmitButton = () => screen.getByTestId(CANCEL_IDS.SUBMIT)
        const getFeedback = () => screen.getByTestId(CANCEL_IDS.FEEDBACK)

        test("with invalid user", () => {
            const tree = render(false, false)
            // expect(tree).toMatchSnapshot()

            expect(getBackButton).toThrow()
            expect(getPlanInfo).toThrow()
            expect(getWarning).toThrow()
            expect(getSubmitButton).toThrow()
            expect(getFeedback).toThrow()
        })

        test("with a valid user who has no plan or has payment not enabled", () => {
            const tree = render(true, false)
            // expect(tree).toMatchSnapshot()

            expect(getBackButton).toThrow()
            expect(getPlanInfo).toThrow()
            expect(getWarning).toThrow()
            expect(getSubmitButton).toThrow()
            expect(getFeedback).toThrow()
        })

        test("valid user with payment enabled and a plan", () => {
            const tree = render(true, true)
            // expect(tree).toMatchSnapshot()

            getBackButton()
            getPlanInfo()
            getSubmitButton()
            getFeedback()
        })
    })

    describe("lets you modify state", () => {
        describe("to cancel", () => {
            test.skip("and provides warning on failure", () => {
                throw new Error("implement please")
            })

            test.skip("and succeeds on success", () => {
                throw new Error("implement please")
            })
        })

        test.skip("to provide feedback", () => {
            throw new Error("implement please")
        })
    })
})

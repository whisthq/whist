/* eslint-disable @typescript-eslint/no-unused-vars */
import React from "react"

import { screen } from "@testing-library/react"
import { removeNondeterminism } from "testing/utils/utils"
import { customRender } from "testing/utils/customRender"
import {
    invalidUser,
    validUserCanLogin,
    validStripeInfo,
    invalidStripeInfo,
    startingAuthFlow,
} from "testing/utils/testState"
import { chainAssign } from "testing/utils/testHelpers"
import Billing from "pages/dashboard/settings/payment/billing/billing"
import { PAYMENT_IDS } from "testing/utils/testIDs"

describe("<Billing />", () => {
    const paymentFlow = (plan: any) => {
        return {
            plan: plan,
        }
    }

    const state = (user: boolean, stripe: boolean, payment: boolean) =>
        chainAssign([
            {
                AuthReducer: {
                    user: user ? validUserCanLogin : invalidUser,
                    authFlow: startingAuthFlow("Log in"),
                },
            },
            {
                DashboardReducer: {
                    stripeInfo: stripe ? validStripeInfo : invalidStripeInfo,
                    paymentFlow: paymentFlow(payment ? "Monthly" : undefined),
                },
            },
        ])

    const render = (user: boolean, plan: boolean) =>
        removeNondeterminism(
            customRender(<Billing testDate={true} />, state(user, true, plan))
        )

    const getPaymentOptions = () => screen.getByTestId(PAYMENT_IDS.PAY)
    // const getCardForm = () => screen.getByTestId(PAYMENT_IDS.CARD_FORM)
    const getInfoBoxes = () => screen.getByTestId(PAYMENT_IDS.INFO)
    const getSubmitButton = () => screen.getByTestId(PAYMENT_IDS.SUBMIT)
    const getWarning = () => screen.getByTestId(PAYMENT_IDS.WARN)

    // const getPlanForm = () => screen.getByTestId(PAYMENT_IDS.PLAN)
    // const getPaymentForm = () => screen.getByTestId(PAYMENT_IDS.PAY)

    describe("renders correctly", () => {
        const tests = [
            ["with an invalid user", [false, false]],
            ["with a valid user and no plan", [true, false]],
            ["with a valid user with a plan", [true, true]],
        ]
        tests.forEach((tuple: any) => {
            const [text, bools] = tuple
            test(text, async () => {
                const [user, plan] = bools

                const tree = render(user, plan)
                // expect(tree).toMatchSnapshot()

                if (user && plan) {
                    getPaymentOptions()
                    // getCardForm()
                    getInfoBoxes()
                    getSubmitButton()
                    // getPaymentForm()
                    // getPlanForm()
                } else {
                    // expect(getPaymentOptions).toThrow()
                    // expect(getCardForm).toThrow()
                    expect(getInfoBoxes).toThrow()
                    expect(getSubmitButton).toThrow()
                    expect(getWarning).toThrow()
                }
            })
        })
    })

    describe("allows you to enter information", () => {
        test.skip("regarding your card", async () => {
            throw new Error("implement please")
        })

        test.skip("and submit", async () => {
            throw new Error("implement please")
        })
    })
})

/* eslint-disable @typescript-eslint/no-unused-vars */
import React from "react"
// import {} from "jest"

import { screen } from "@testing-library/react"
import { removeNondeterminism } from "testing/utils/utils"
import { customRender } from "testing/utils/customRender"
import { CONFIRMATION_IDS } from "testing/utils/testIDs"
import Confirmation from "pages/dashboard/settings/payment/confirmation/confirmation"
import {
    invalidUser,
    validUserCanLogin,
    invalidStripeInfo,
    validStripeInfo,
} from "testing/utils/testState"
import { chainAssign } from "testing/utils/testHelpers"

describe("<Confirmation />", () => {
    const stripeInfo = (obj: Object) => {
        return { DashboardReducer: { stripeInfo: obj } }
    }
    const authReducer = (obj: Object) => {
        return { AuthReducer: { user: obj } }
    }

    const state = (user: boolean, plan: boolean) =>
        chainAssign([
            authReducer(user ? validUserCanLogin : invalidUser),
            stripeInfo(plan ? validStripeInfo : invalidStripeInfo),
        ])

    const render = (user: boolean, plan: boolean) =>
        removeNondeterminism(customRender(<Confirmation />, state(user, plan)))

    const getBackButton = () => screen.getByTestId(CONFIRMATION_IDS.BACK)
    const getPlanInfo = () => screen.getByTestId(CONFIRMATION_IDS.INFO)

    describe("renders correctly", () => {
        test("without valid user", async () => {
            // will have some randomness due to trial end
            const tree = render(false, false)
            // expect(tree).toMatchSnapshot()

            expect(getBackButton).toThrow()
            expect(getPlanInfo).toThrow()
        })

        test("with a valid user but no payment information", async () => {
            const tree = render(true, false)
            // expect(tree).toMatchSnapshot()

            expect(getBackButton).toThrow()
            expect(getPlanInfo).toThrow()
        })

        test("with a valid user with payment information", async () => {
            const tree = render(true, true)
            // expect(tree).toMatchSnapshot()

            getBackButton()
            getPlanInfo()
        })
    })
})

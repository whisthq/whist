/* eslint-disable @typescript-eslint/no-unused-vars */
import React from "react"
// import {} from "jest"

import { screen } from "@testing-library/react"
import { removeNondeterminism } from "testing/utils/utils"
import { customRender } from "testing/utils/customRender"
import { CANCELED_IDS } from "testing/utils/testIDs"
import Canceled from "pages/dashboard/settings/payment/cancel/canceled"
import { invalidUser, validUserCanLogin } from "testing/utils/testState"
import { singleAssign } from "testing/utils/testHelpers"

describe("<Canceled />", () => {
    const state = (user: boolean) =>
        singleAssign({
            AuthReducer: { user: user ? validUserCanLogin : invalidUser },
        })

    const render = (user: boolean) =>
        removeNondeterminism(customRender(<Canceled />, state(user)))

    const getBackButton = () => screen.getByTestId(CANCELED_IDS.BACK)
    const getPlanInfo = () => screen.getByTestId(CANCELED_IDS.INFO)
    const getAddButton = () => screen.getByTestId(CANCELED_IDS.ADD)

    describe("renders correctly", () => {
        test("with an invalid user", async () => {
            const tree = render(false)
            // expect(tree).toMatchSnapshot()

            expect(getBackButton).toThrow()
            expect(getPlanInfo).toThrow()
            expect(getAddButton).toThrow()
        })

        test("with a valid user", async () => {
            const tree = render(true)
            // expect(tree).toMatchSnapshot()

            getBackButton()
            getPlanInfo()
            getAddButton()
        })
    })
})

/* eslint-disable @typescript-eslint/no-unused-vars */
import React from "react"

import { screen } from "@testing-library/react"
import { removeNondeterminism } from "testing/utils/utils"
import { customRender } from "testing/utils/customRender"
import Plan from "pages/dashboard/settings/payment/plan/plan"
import { invalidUser, validUserCanLogin } from "testing/utils/testState"
import { chainAssign } from "testing/utils/testHelpers"
import { PLAN_IDS } from "testing/utils/testIDs"

describe("<Plan />", () => {
    describe("renders correctly", () => {
        const paymentFlow = (obj: any) => {
            return { plan: obj }
        }

        const state = (user: boolean, isChecked: boolean) =>
            chainAssign([
                {
                    AuthReducer: {
                        user: user ? validUserCanLogin : invalidUser,
                    },
                },
                {
                    DashboardReducer: {
                        paymentFlow: isChecked
                            ? paymentFlow("Starter")
                            : paymentFlow(undefined),
                    },
                },
            ])

        const render = (user: boolean, isChecked: boolean) =>
            removeNondeterminism(customRender(<Plan />, state(user, isChecked)))

        const getInfo = () => screen.getByTestId(PLAN_IDS.INFO)
        const getNext = () => screen.getByTestId(PLAN_IDS.NEXT)
        const getBoxes = () => screen.getByTestId(PLAN_IDS.BOXES)

        test("invalid user", async () => {
            const tree = render(false, false)

            // expect(tree).toMatchSnapshot()

            expect(getInfo).toThrow()
            expect(getNext).toThrow()
            expect(getBoxes).toThrow()
        })

        test("valid user", async () => {
            const tree = render(true, false)

            // expect(tree).toMatchSnapshot()

            getInfo()
            getNext()
            getBoxes()
        })
    })

    describe("allows you to select different plans", () => {
        test.skip("and set checked plan", async () => {
            throw new Error("implement please")
        })

        test.skip("and continue to payment once you've checked a plan", async () => {
            throw new Error("implement please")
        })
    })
})

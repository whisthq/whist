/* eslint-disable @typescript-eslint/no-unused-vars */
import React from "react"
// import {} from "jest"
import { screen, act, cleanup } from "@testing-library/react"
import Dashboard from "pages/dashboard/dashboard"
import {
    validUserCanLogin,
    invalidUser,
    startingAuthFlow,
} from "testing/utils/testState"

import { HEADER, DASHBOARD_IDS } from "testing/utils/testIDs"

import { customRender } from "testing/utils/customRender"
import { singleAssign } from "testing/utils/testHelpers"
import { removeNondeterminism, zeroMsWait } from "testing/utils/utils"

// TODO MAKE SURE SNAPSHOTS ARE NOT UNDEFINED
describe("<Dashboard />", () => {
    // a render that will be specifically userful for this test
    const render = (node: any, assign: Object) =>
        customRender(
            node,
            singleAssign({
                AuthReducer: {
                    user: assign,
                    authFlow: startingAuthFlow("Forgot"),
                },
            })
        )

    describe("renders correctly", () => {
        // screen.getBy will throw new Error is there is no matching elements
        const getPuffAnimation = () => screen.getByTestId(DASHBOARD_IDS.PUFF)
        const getHeader = () => screen.getByTestId(HEADER)
        const getRight = () => screen.getByTestId(DASHBOARD_IDS.RIGHT)
        test("when it is loading", async () => {
            await act(async () => {
                removeNondeterminism(
                    render(<Dashboard user={validUserCanLogin} />, {
                        loading: true,
                    })
                )
                await zeroMsWait()

                // expect(tree).toMatchSnapshot()
            })
            await zeroMsWait()

            //note that it doesn't need loading to be set to true!
            // getPuffAnimation()

            // in the future we may still want to display the puff animation
            // both for consistency and ease of navigation for users
            // expect(getBackToHome).toThrow()
            expect(getRight).toThrow()
        })

        test("when the user is valid and can login", async () => {
            await act(async () => {
                removeNondeterminism(
                    render(
                        <Dashboard
                            user={{ ...validUserCanLogin, canLogin: false }}
                        />,
                        { ...validUserCanLogin, canLogin: false }
                    )
                )
                await zeroMsWait()

                // expect(tree).toMatchSnapshot()
            })
            await zeroMsWait()

            getHeader()
            getRight()

            expect(getPuffAnimation).toThrow()
        })

        test("when the user is invalid", async () => {
            await act(async () => {
                removeNondeterminism(
                    render(<Dashboard user={invalidUser} />, invalidUser)
                )
                await zeroMsWait()

                // expect(tree).toMatchSnapshot()
            })
            await zeroMsWait()

            // this is a redirect
            // getPuffAnimation()
            // expect(getPuffAnimation).toThrow()
            // expect(getDownloadBox).toThrow()
        })
    })

    describe("manipulates canLogin correctly", () => {
        test.skip("when the user is offboarded", () => {
            throw new Error("implement please")
        })

        test.skip("when the user is an admin", () => {
            throw new Error("implement please")
        })
    })
})

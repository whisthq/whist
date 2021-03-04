/* eslint-disable @typescript-eslint/no-unused-vars */
import React from "react"
import { removeNondeterminism } from "testing/utils/utils"
import { customRender } from "testing/utils/customRender"
import { rootState } from "testing/utils/testHelpers"
import Reset from "pages/auth/reset/reset"
import { screen, fireEvent, cleanup } from "@testing-library/react"
import { waitFor } from "@testing-library/dom"
import { setupServer } from "msw/node"
import { msWait } from "testing/utils/utils"
import {
    invalidUser,
    startingAuthFlow,
    validUserCanLogin,
    validFractalUser,
} from "testing/utils/testState"
import { chainAssign, generateUser } from "testing/utils/testHelpers"
import { handlers } from "testing/utils/testServer"
import Auth from "pages/auth/auth"
import ResetView from "pages/auth/reset/components/resetView"
import { HEADER, AUTH_IDS, RESET_IDS } from "testing/utils/testIDs"
import PLACEHOLDER from "shared/constants/form"

// declare which API requests to mock
const server = setupServer(...handlers)

beforeAll(() => server.listen())
// reset any request handlers that are declared as a part of our tests
// (i.e. for testing one-time error scenarios)
afterEach(() => server.resetHandlers())
// clean up once the tests are done
afterAll(() => server.close())

const resetUser = {
    passwordResetEmail: "test@test.co",
    passwordResetToken: "testing",
    resetTokenStatus: "verified",
}

describe("<Reset />", () => {
    // getter functions for dom elements
    const getEmailField = () => screen.getByPlaceholderText(PLACEHOLDER.EMAIL)
    const getConfirmPasswordField = () =>
        screen.getByPlaceholderText(PLACEHOLDER.CONFIRMPASSWORD)
    const getPasswordField = () =>
        screen.getByPlaceholderText(PLACEHOLDER.PASSWORD)

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

    // state for mocking reset tests
    const resetState = () =>
        chainAssign([
            {
                AuthReducer: {
                    authFlow: {
                        passwordResetEmail: resetUser.passwordResetEmail,
                        passwordResetToken: resetUser.passwordResetToken,
                        resetTokenStatus: resetUser.resetTokenStatus,
                    },
                },
            },
        ])

    // renders the reset password view
    const resetViewRender = (
        useState: boolean,
        token?: string,
        validToken?: boolean
    ) => {
        const tree = customRender(
            <ResetView token={token} validToken={validToken} />,
            useState ? resetState() : undefined
        )
        return { original: tree }
    }
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

    describe("renders correctly", () => {
        const resetRender = (validToken: boolean) =>
            removeNondeterminism(
                customRender(
                    <Reset testSearch={validToken ? "token" : ""} />,
                    rootState
                )
            )

        const getHeader = () => screen.getByTestId(HEADER)
        const getLoading = () => screen.getByTestId(RESET_IDS.LOAD)
        const getFailure = () => screen.getByTestId(RESET_IDS.FAIL)
        const getButton = () => screen.getByTestId(RESET_IDS.BUTTON)
        const getForm = () => screen.getByTestId(RESET_IDS.FORM)
        const getSuccess = () => screen.getByTestId(RESET_IDS.SUCCESS)

        test("with an invalid token", async () => {
            resetRender(false)
            // expect(tree).toMatchSnapshot()

            expect(getLoading).toThrow()
            expect(getFailure).toThrow()
            expect(getButton).toThrow()
            expect(getForm).toThrow()
            expect(getSuccess).toThrow()
        })

        describe("with a valid token", () => {
            test("while loading", async () => {
                resetRender(true)
                // expect(tree).toMatchSnapshot()

                getHeader()
                getLoading()
            })

            test.skip("while filling out form", async () => {
                throw new Error("please implement")
            })

            test.skip("on an unsuccesfful form send", async () => {
                throw new Error("please implement")
            })

            test.skip("on a successful form send", async () => {
                throw new Error("please implement")
            })
        })
    })

    test("submit forgot password request", async () => {
        const getButton = () => screen.getByTestId(AUTH_IDS.BUTTON)

        // renders forgot view with a given email token to simulate
        // resetting password
        render(
            "Forgot",
            undefined,
            undefined,
            undefined,
            validFractalUser.emailToken
        )

        const emailField = getEmailField()

        let button = getButton()

        await fireEvent.change(emailField, {
            target: { value: validFractalUser.user_id },
        })
        await fireEvent.click(button.firstChild)
        await msWait(500)
        let resetToken = ""
        await waitFor(async () => {
            await msWait(500)
            expect(button).not.toBeInTheDocument()
            expect(emailField).not.toBeInTheDocument()
            resetToken = screen.getByTestId(AUTH_IDS.RESETTOKEN).textContent
        })

        await msWait(500)
        cleanup()

        resetViewRender(false, resetToken, true)

        await msWait(1000)
        const passwordField = getPasswordField()
        const confirmPasswordField = getConfirmPasswordField()

        button = screen.getByTestId(RESET_IDS.BUTTON)
        const { password } = generateUser()
        await fireEvent.change(passwordField, {
            target: { value: password },
        })
        await fireEvent.change(confirmPasswordField, {
            target: { value: password },
        })
        await fireEvent.click(button.firstChild)

        await msWait(1000)

        await waitFor(async () => {
            screen.getByTestId(RESET_IDS.SUCCESS)
        })
    })
})

import React from "react"
import { screen, fireEvent, cleanup } from "@testing-library/react"
import { config } from "shared/constants/config"
import { rest } from "msw"
import { setupServer } from "msw/node"
import { waitFor } from "@testing-library/dom"
import { msWait } from "testing/utils/utils"
import { customRender } from "testing/utils/customRender"
import {
    invalidUser,
    startingAuthFlow,
    validUserCanLogin,
    invalidTestUser,
    verifiedUser,
    unverifiedUser,
} from "testing/utils/testState"
import { handlers } from "testing/utils/testServer"
import { chainAssign, generateUser } from "testing/utils/testHelpers"
import Auth from "pages/auth/auth"
import VerifyView from "pages/auth/verify/components/verifyView"
import { HEADER, AUTH_IDS, VERIFY_IDS } from "testing/utils/testIDs"
import PLACEHOLDER from "shared/constants/form"

// declare which API requests to mock
const server = setupServer(...handlers)

beforeAll(() => server.listen())
// reset any request handlers that are declared as a part of our tests
// (i.e. for testing one-time error scenarios)
afterEach(() => server.resetHandlers())
// clean up once the tests are done
afterAll(() => server.close())

describe("<Auth />", () => {
    /**
     * creates a new state variable with a signed in user, with given userID,
     * accessToken, and refreshToken
     *
     * @param userID: email of a given user for testing
     * @param accessToken: access token of a given user
     * @param refreshToken: refresh token of a given user
     *  */
    const verifyState = (
        userID: string,
        accessToken: string,
        refreshToken: string
    ) =>
        chainAssign([
            {
                AuthReducer: {
                    user: {
                        userID,
                        accessToken,
                        refreshToken,
                    },
                    authFlow: startingAuthFlow("Sign up"),
                },
            },
        ])
    /**
     * Sets up a state for loading the auth component
     *
     * @param mode: the mode of the auth component - Sign up, Log in, or Forgot
     * @param isValid if we want to test with a valid or invalid user
     */
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

    /**
     * Sets up state variable for loading the verifyview component
     *
     * @param emailToken email token for verifying password
     * @param userID email of the test user
     * @param accessToken access token of a given user
     * @param refreshToken refresh token of a given user
     */
    const verifyRender = (
        emailToken: string,
        userID: string,
        accessToken: string,
        refreshToken: string
    ) => {
        customRender(
            <VerifyView validToken={true} token={emailToken} />,
            verifyState(userID, accessToken, refreshToken)
        )
    }
    // we can set default to true since !stripeInfo.plan || ...
    /**
     * Renders auth component
     *
     * @param mode mode to render the auth component in - forgot, log in, or sign up
     * @param testLocation given a test location endpoint for callback testing
     * @param isValid if we want to test with a valid user
     * @param testSignup if we want to test signup
     * @param emailToken a valid email token for testing reset password
     */
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
        const get: { [id: string]: () => void } = {}
        const vids: { [id: string]: string } = AUTH_IDS // typescript
        vids["header"] = HEADER

        Object.keys(AUTH_IDS).forEach((id: any) => {
            // skip the side switch link for now
            if (id !== "SWITCHLINK") {
                get[AUTH_IDS[id]] = () => screen.getByTestId(vids[id])
            }
        })

        const modes = ["Log in", "Sign up", "Forgot"]
        const modeIds: { [key: string]: string[] } = {
            "Log in": [
                "header",
                AUTH_IDS.FORM,
                AUTH_IDS.BUTTON,
                AUTH_IDS.SWITCH,
                AUTH_IDS.LOGIN,
            ],
            "Sign up": [
                "header",
                AUTH_IDS.FORM,
                AUTH_IDS.BUTTON,
                AUTH_IDS.SWITCH,
                AUTH_IDS.SIGNUP,
            ],
            Forgot: [
                "header",
                AUTH_IDS.FORM,
                AUTH_IDS.BUTTON,
                AUTH_IDS.SWITCH,
                AUTH_IDS.FORGOT,
            ],
        }

        const check = (mode: string, checkHeader: boolean = true) => {
            const desiredChecks = mode in modeIds ? modeIds[mode] : []
            Object.keys(get).forEach((id: string) => {
                if (id !== "header" || !checkHeader) {
                    if (desiredChecks.includes(id)) {
                        get[id]()
                    } else {
                        expect(get[id]).toThrow()
                    }
                }
            })
        }

        modes.forEach((mode: string) => {
            test(mode + " mode", async () => {
                render(mode)
                check(mode)
            })
        })
    })

    // getter functions for dom elements
    const getEmailField = () => screen.getByPlaceholderText(PLACEHOLDER.EMAIL)
    const getConfirmPasswordField = () =>
        screen.getByPlaceholderText(PLACEHOLDER.CONFIRMPASSWORD)
    const getPasswordField = () =>
        screen.getByPlaceholderText(PLACEHOLDER.PASSWORD)
    const getNameField = () => screen.getByPlaceholderText(PLACEHOLDER.NAME)
    const getButton = () => screen.getByTestId(AUTH_IDS.BUTTON)

    describe("lets you", () => {
        test("login", async () => {
            const { original } = render("Log in")

            // get relevant fields
            const emailField = getEmailField()
            const passwordField = getPasswordField()
            const button = getButton()

            // fill out form
            fireEvent.change(emailField, {
                target: { value: verifiedUser.userID },
            })
            fireEvent.change(passwordField, {
                target: { value: verifiedUser.password },
            })
            await fireEvent.click(button.firstChild)
            await msWait(2000)

            await waitFor(async () => {
                expect(original.history.location.pathname).toBe("/dashboard")
                expect(button).not.toBeInTheDocument()
                expect(passwordField).not.toBeInTheDocument()
                expect(emailField).not.toBeInTheDocument()
            })
        })

        test("signup", async () => {
            render("Sign up", undefined, undefined, true)
            // get relevant fields
            const emailField = getEmailField()
            const passwordField = getPasswordField()
            const nameField = getNameField()
            const confirmPasswordField = getConfirmPasswordField()
            const button = getButton()

            // generate random user
            const { email, password, name } = generateUser()

            // fills out relevant fields
            await fireEvent.change(emailField, { target: { value: email } })
            await fireEvent.change(nameField, { target: { value: name } })
            await fireEvent.change(passwordField, {
                target: { value: password },
            })
            await fireEvent.change(confirmPasswordField, {
                target: { value: password },
            })

            await fireEvent.click(button.firstChild)
            await msWait(2000)
            // gets returns tokens from json
            let emailToken = ""
            let userID = ""
            let refreshToken = ""
            let accessToken = ""
            await waitFor(async () => {
                emailToken = screen.getByTestId(AUTH_IDS.EMAILTOKEN).textContent
                userID = screen.getByTestId(AUTH_IDS.USERID).textContent
                refreshToken = screen.getByTestId(AUTH_IDS.REFRESHTOKEN)
                    .textContent
                accessToken = screen.getByTestId(AUTH_IDS.ACCESSTOKEN)
                    .textContent
            })
            cleanup()
            // checks if the verify view component hsa been rendered
            verifyRender(emailToken, userID, accessToken, refreshToken)
            await msWait(1000)
            screen.getByTestId(VERIFY_IDS.SUCCESS)
        })

        test("switch modes", async () => {
            render("Log in")
            let switchLink = screen.getByText(PLACEHOLDER.SIGNUPSWITCH)
            getEmailField()
            // switches to
            await fireEvent.click(switchLink)

            getButton()
            getNameField()

            switchLink = screen.getByText(PLACEHOLDER.LOGINSWITCH)
            await fireEvent.click(switchLink)
            expect(() =>
                screen.getByPlaceholderText(PLACEHOLDER.CONFIRMPASSWORD)
            ).toThrow()

            switchLink = screen.getByText(PLACEHOLDER.FORGOTSWITCH)
            await fireEvent.click(switchLink.firstChild)
            getEmailField()
        })
    })

    describe("provides", () => {
        test("warnings if you fail login", async () => {
            render("Log in")
            const { email, password } = invalidTestUser
            const emailField = getEmailField()
            const passwordField = getPasswordField()
            const button = getButton()
            fireEvent.change(emailField, { target: { value: email } })
            fireEvent.change(passwordField, { target: { value: password } })

            await fireEvent.click(button.firstChild)

            screen.getByText("Invalid email or password. Try again.")
        })

        test("warnings if you fail signup", async () => {
            render("Sign up")
            const {
                email,
                password,
                confirmPassword,
                name,
                noLower,
                noNumber,
                noUpper,
            } = invalidTestUser
            const emailField = getEmailField()
            const passwordField = getPasswordField()
            const nameField = getNameField()
            const confirmPasswordField = getConfirmPasswordField()
            getButton()

            fireEvent.change(nameField, { target: { value: name } })
            fireEvent.change(nameField, { target: { value: "" } })
            screen.getByText("Required")

            fireEvent.change(emailField, { target: { value: email } })
            screen.getByText("Invalid email")

            await fireEvent.change(passwordField, {
                target: { value: password },
            })
            screen.getByText("Too short")
            await fireEvent.change(passwordField, {
                target: { value: noLower },
            })
            screen.getByText("Needs lowercase letter")
            await fireEvent.change(passwordField, {
                target: { value: noUpper },
            })
            screen.getByText("Needs uppercase letter")
            await fireEvent.change(passwordField, {
                target: { value: noNumber },
            })
            screen.getByText("Needs number")
            await fireEvent.change(confirmPasswordField, {
                target: { value: confirmPassword },
            })
            screen.getByText("Doesn't match")
        })
    })

    describe("redirects correctly", () => {
        test("if you are not verified and should be sent to verification", async () => {
            server.use(
                rest.post(
                    config.url.WEBSERVER_URL + "/account/login",
                    (req, res, ctx) => {
                        // Persist user's authentication in the session
                        return res.once(
                            ctx.json({
                                access_token: "accessToken",
                                can_login: true,
                                is_user: true,
                                name: null,
                                encrypted_config_token:
                                    "f279a50d85c6f514aa6f634f0345cf85f913279da804f41438bf7967c666db527b8b5df9bc07333699541d4be4bdfa8eba99e2ee699d7d82f1c5b51bd119fe698c80b0bedef32f1a3e9e99",
                                refresh_token: "refreshToken",
                                verification_token: "verificationToken",
                                verified: false,
                            })
                        )
                    }
                )
            )
            const { original } = render("Log in")
            const emailField = getEmailField()
            const passwordField = getPasswordField()
            const button = getButton()
            fireEvent.change(emailField, {
                target: { value: unverifiedUser.userID },
            })
            fireEvent.change(passwordField, {
                target: { value: unverifiedUser.password },
            })
            await fireEvent.click(button.firstChild)
            await msWait(1000)

            await waitFor(async () => {
                expect(original.history.location.pathname).toBe("/verify")

                expect(() => getEmailField()).toThrow()
                expect(() => getPasswordField()).toThrow()
            })
        })

        test("if you are already logged in", async () => {
            let { original } = render("Log in")
            const emailField = getEmailField()
            const passwordField = getPasswordField()
            const button = getButton()
            fireEvent.change(emailField, {
                target: { value: "steveli@college.harvard.edu" },
            })
            fireEvent.change(passwordField, { target: { value: "Wavelf2020" } })

            await fireEvent.click(button.firstChild)
            await msWait(2000)

            await waitFor(async () => {
                expect(original.history.location.pathname).toBe("/dashboard")
            })
        })
    })
})

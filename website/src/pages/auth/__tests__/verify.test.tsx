/* eslint-disable @typescript-eslint/no-unused-vars */
import React from "react"
import { screen } from "@testing-library/react"
import { removeNondeterminism, removeRiskyProps } from "testing/utils/utils"
import { customRender } from "testing/utils/customRender"
import {
    invalidUser,
    validUserCanLogin,
    startingAuthFlow,
} from "testing/utils/testState"
import { singleAssign } from "testing/utils/testHelpers"
import Verify from "pages/auth/verify/verify"
import { HEADER, VERIFY_IDS } from "testing/utils/testIDs"

describe("<Verify />", () => {
    describe("renders correctly", () => {
        const get: { [id: string]: () => void } = {}
        const vids: { [id: string]: string } = VERIFY_IDS // typescript
        vids["header"] = HEADER

        for (const id in Object.keys(VERIFY_IDS)) {
            get[id] = () => screen.getByTestId(vids[id])
        }

        // const userCopy = deepCopy(validUserCanLogin)
        // userCopy.emailVerified = false

        const state = (isValidUser: boolean) =>
            singleAssign({
                AuthReducer: {
                    user: isValidUser ? validUserCanLogin : invalidUser,
                    authFlow: startingAuthFlow("Sign up"),
                },
            })

        // we can set default to true since !stripeInfo.plan || ...
        const render = (isValidUser: boolean, validToken: boolean) =>
            removeNondeterminism(
                customRender(
                    <Verify validToken={validToken} />,
                    state(isValidUser)
                )
            )

        const checks = [
            ["invalid user", [], false, false],
            ["valid user that is already verified", [], true, true],
            [
                "valid user and an invalid token",
                [
                    "header",
                    VERIFY_IDS.BACK,
                    VERIFY_IDS.INVALID,
                    VERIFY_IDS.RETRY,
                ],
                true,
                false,
            ],
            [
                "valid user while processing",
                ["header", VERIFY_IDS.LOAD],
                true,
                true,
            ],
            [
                "valid user on success",
                ["header", VERIFY_IDS.SUCCESS],
                true,
                true,
            ],
            ["valid user on failure", ["header", VERIFY_IDS.FAIL], true, true],
        ]

        const check = (
            desiredChecks: string[],
            checkHeader: boolean = true
        ) => {
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

        checks.forEach((element: any) => {
            const [text, desiredChecks, validatedUser, validToken] = element
            test(text, async () => {
                const tree = render(validatedUser, validToken)

                // const apply = (str: string) => removeRiskyProps(str)

                // // await customSnapshot(
                // //     tree,
                // //     `./src/pages/auth/__tests__/__snapshots__/custom-verify-${text}`,
                // //     apply
                // // )

                check(desiredChecks, desiredChecks.length > 0)
            })
        })
    })

    describe("is able to communicate with server", () => {
        test.skip("and verify a valid token", async () => {
            throw new Error("please implement")
        })

        test.skip("and fail an invalid token", async () => {
            throw new Error("please implement")
        })
    })
})

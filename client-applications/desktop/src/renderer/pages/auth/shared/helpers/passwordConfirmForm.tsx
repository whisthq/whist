import React from "react"

import { InputWarning } from "@app/components/custom/warning"
import { FractalInput, FractalInputState } from "@app/components/html/input"

import {
    checkPasswordVerbose,
    checkPassword,
    checkConfirmPasswordVerbose,
} from "@app/renderer/pages/auth/shared/helpers/authHelpers"

export const PasswordConfirmForm = (props: {
    onEnterKey?: () => void
    password: string
    setPassword: (password: string) => void
    confirmPassword: string
    setConfirmPassword: (confirmPassword: string) => void
}) => {
    const {
        onEnterKey,
        password,
        setPassword,
        confirmPassword,
        setConfirmPassword,
    } = props

    return (
        <div>
            <InputWarning warning={checkPasswordVerbose(password)} />
            <h5 className="font-body text-left font-semibold mt-7 text-sm">
                Password
            </h5>
            <FractalInput
                type="password"
                placeholder="Password"
                onChange={(password: string) => setPassword(password)}
                onEnterKey={onEnterKey}
                value={password}
                state={
                    checkPassword(password)
                        ? FractalInputState.SUCCESS
                        : FractalInputState.DEFAULT
                }
                className="mt-1"
            />
            <InputWarning
                warning={checkConfirmPasswordVerbose(password, confirmPassword)}
            />
            <h5 className="font-body text-left font-semibold mt-7 text-sm">
                Confirm Password
            </h5>
            <FractalInput
                type="password"
                placeholder="Confirm Password"
                onChange={(confirmPassword: string) =>
                    setConfirmPassword(confirmPassword)
                }
                onEnterKey={onEnterKey}
                value={confirmPassword}
                state={
                    checkPassword(confirmPassword)
                        ? FractalInputState.SUCCESS
                        : FractalInputState.DEFAULT
                }
                className="mt-1"
            />
        </div>
    )
}

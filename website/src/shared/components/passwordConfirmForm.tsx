import React from "react"

import Input from "shared/components/input"

import { checkPassword } from "pages/auth/constants/authHelpers"

import "styles/auth.css"

/**
 * This is a simple form meant to help us do a confirm-password form (given that there is a lot of overlap).
 * It's simply two inputs representing a password and a confirm password. It checks that the two passwords match
 * and that they are both valid. This is useful since we reuse this functionality both in reset and in sign up.
 *
 * @param props will include some callbacks and values needed for the form to work. Since variables are stored in
 * the parent component we will need:
 *
 * changePassword - a function that changes the value of the password
 * changeConfirmPassword - a function that changes the value of the confirm password
 * onKeyPress - a function that basically checks for when you click enter
 * password - the value of the password
 * confirmPassword - the value of the password confirming
 * passwordWarning? - a warning (optional) to show for the password
 * confirmPasswordWarning? - a warning (optional) to display for the confirm password
 * isFirstElement? - whether it's the first element, (default no), if so will have a 40 margin top on first input not 13
 *
 * Note that we store variables in the parent component since we need to them or actual **logic**
 * We'd like to have logic in one place to make it easier to work with.
 */
const PasswordConfirmForm = (props: {
    changePassword: (evt: any) => any
    changeConfirmPassword: (evt: any) => any
    onKeyPress: (evt: any) => any
    password: string
    confirmPassword: string
    passwordWarning?: string
    confirmPasswordWarning?: string
    isFirstElement?: boolean
    profile?: boolean
}) => {
    const {
        changePassword,
        changeConfirmPassword,
        onKeyPress,
        password,
        confirmPassword,
        passwordWarning,
        confirmPasswordWarning,
        isFirstElement,
        profile,
    } = props

    return (
        <div style={{ width: "100%" }}>
            <div style={{ marginTop: isFirstElement ? 40 : 13 }}>
                <Input
                    text={profile ? "New Password" : "Password"}
                    type="password"
                    placeholder={profile ? "New Password" : "Password"}
                    onChange={changePassword}
                    onKeyPress={onKeyPress}
                    value={password}
                    warning={passwordWarning}
                    valid={checkPassword(password)}
                />
            </div>
            <div style={{ marginTop: 13 }}>
                <Input
                    text="Confirm Password"
                    type="password"
                    placeholder="Password"
                    onChange={changeConfirmPassword}
                    onKeyPress={onKeyPress}
                    value={confirmPassword}
                    warning={confirmPasswordWarning}
                    valid={
                        confirmPassword.length > 0 &&
                        confirmPassword === password &&
                        checkPassword(password)
                    }
                />
            </div>
        </div>
    )
}

export default PasswordConfirmForm

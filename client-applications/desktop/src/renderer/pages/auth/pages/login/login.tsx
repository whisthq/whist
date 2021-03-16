import React, { useState, ChangeEvent, KeyboardEvent } from "react"

import { Logo } from "@app/components/logo"
import { BaseInput, AuthInput } from "@app/components/html/input"
import { AuthWarning } from "@app/components/custom/warning"
import { BaseButton } from "@app/components/html/button"

import {
    loginEnabled,
    checkEmail,
    checkPassword,
} from "@app/renderer/pages/auth/shared/helpers/authHelpers"
import { emailLogin } from "@app/renderer/pages/auth/pages/login/shared/helpers/api"

import FractalKey from "@app/@types/input"

const Login = (props: { onLogin: (json: object) => void }) => {
    const { onLogin } = props

    const [email, setEmail] = useState("")
    const [password, setPassword] = useState("")
    const [processing, setProcessing] = useState(false)
    const [loginWarning, setLoginWarning] = useState("")

    const login = () => {
        if (loginEnabled(email, password)) {
            setProcessing(true)
            emailLogin(email, password).then(({ json }) => {
                if (json && json.access_token) {
                    onLogin(json)
                } else {
                    setLoginWarning("Invalid email or password provided.")
                }
                setProcessing(false)
            })
        } else {
            setLoginWarning("Invalid email or password provided.")
            setProcessing(false)
        }
    }

    // Handles the ENTER key
    // TODO: Wrap this logic in a function
    const onKeyPress = (evt: KeyboardEvent) => {
        if (evt.key === FractalKey.ENTER) {
            setProcessing(true)
            login()
        }
    }

    // Handles email form keystrokes
    const changeEmail = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setEmail(evt.target.value)
    }

    // Handles password form keystrokes
    const changePassword = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setPassword(evt.target.value)
    }

    return (
        <div className="flex flex-col justify-center items-center bg-white h-screen px-8">
            <Logo />
            <h5 className="font-body mt-6 text-xl">
                Please log in into your account
            </h5>
            <AuthWarning warning={loginWarning} />
            <AuthInput
                text="Email"
                type="email"
                placeholder="Email"
                onChange={changeEmail}
                onKeyPress={onKeyPress}
                value={email}
                valid={checkEmail(email)}
                className="mt-7"
            />
            <AuthInput
                text="Password"
                type="password"
                placeholder="Password"
                onChange={changePassword}
                onKeyPress={onKeyPress}
                value={password}
                valid={checkPassword(password)}
                className="mt-4"
            />
            <BaseButton
                label="Sign In"
                className="mt-5 w-full"
                processing={processing}
                onClick={login}
            />
        </div>
    )
}

export default Login

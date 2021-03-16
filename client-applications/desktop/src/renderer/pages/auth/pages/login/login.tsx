import React from "react"

import { Logo } from "@app/components/logo"
import { BaseInput } from "@app/components/base/input"
import { BaseButton } from "@app/components/base/button"

const Login = (props: {
    onLogin: () => void
}) => {
    const {onLogin} = props 
    
    return (
        <div className="flex flex-col justify-center items-center bg-white h-screen px-8">
            <Logo />
            <div className="w-full space-y-4 mt-8">
                <div>
                    <h5 className="text-gray font-body text-sm font-semibold mb-1">
                        Email
                    </h5>
                    <BaseInput className="w-full" placeholder="bob@gmail.com" />
                </div>
                <div>
                    <h5 className="text-gray font-body text-sm font-semibold mb-1">
                        Password
                    </h5>
                    <BaseInput
                        type="password"
                        className="w-full"
                        placeholder="Password"
                    />
                </div>
                <div>
                    <BaseButton label="Sign In" className="mt-2 mb-4 w-full" />
                </div>
            </div>
        </div>
    )
}

export default Login

import React from "react"
import ReactDOM from "react-dom"

import classNames from "classnames"
import "@app/styles/global.module.css"
import "@app/styles/tailwind.css"
import { Logo } from "@app/components/logo"
import { BaseInput } from "@app/components/input"
import { BaseButton } from "@app/components/button"


const RootComponent = () => (
    <div className="flex flex-col justify-center items-center bg-gray-800 h-screen">
        <Logo className="" dark/>
        <div className="w-64 space-y-4 mt-8">
            <div>
                <h5 className="text-white font-body text-sm">Email</h5>
                <BaseInput className="w-full"/>
            </div>
            <div>
                <h5 className="text-white font-body text-sm">Password</h5>
                <BaseInput type="password" className="w-full"/>
            </div>
            <div>
                <BaseButton label="Sign In"
                            className="bg-gray-600 mt-8 mb-4 w-full text-white"/>
            </div>
        </div>
    </div>
)

ReactDOM.render(<RootComponent />, document.getElementById("root"))

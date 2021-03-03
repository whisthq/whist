import React, { FC } from "react"

import LogoPurple from "assets/icons/logoPurple.png"
import history from "shared/utils/history"

const AuthContainer = (props: {
    title?: string
    children: JSX.Element | JSX.Element[]
}) => {
    const {title, children} = props

    const goHome = () => {
        history.push("/")
    }

    return(
        <div className="w-96 m-auto relative top-32 min-h-screen">
            <img src={LogoPurple} className="w-12 h-12 m-auto cursor-pointer" onClick={goHome}/>
            {title && (
                 <div className="text-3xl font-medium text-center mt-12">{title}</div>
            )}
            {children}
        </div>
    )
}

export default AuthContainer
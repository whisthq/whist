import React, { useContext } from "react"

import { ScreenSize } from "shared/constants/screenSizes"
import MainContext from "shared/context/mainContext"

import history from "shared/utils/history"

export const AuthNavigator = (props: {
    link: string
    redirect: string
    beforeLink?: string
    afterLink?: string
    id?: string
}) => {
    const { width } = useContext(MainContext)

    const { link, redirect, beforeLink, afterLink, id } = props

    const onClick = () => {
        history.push(redirect)
    }

    return (
        <div className="text-center">
            {beforeLink && (`${beforeLink} `)}
            {width < ScreenSize.SMALL && <br />}
            <span
                className="text-blue font-medium cursor-pointer"
                onClick={onClick}
                id={id}
            >
                {link}
            </span>
            {afterLink && afterLink}
        </div>
    )
}

export default AuthNavigator

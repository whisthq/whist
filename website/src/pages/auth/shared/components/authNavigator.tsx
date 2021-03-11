import React from "react"

import history from "shared/utils/history"

export const AuthNavigator = (props: {
    link: string
    redirect: string
    beforeLink?: string
    afterLink?: string
    id?: string
}) => {
    const { link, redirect, beforeLink, afterLink, id } = props

    const onClick = () => {
        history.push(redirect)
    }

    return (
        <div className="text-center">
            {beforeLink && (`${beforeLink} `)}
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

import React, { useContext } from "react"

import { ScreenSize } from "shared/constants/screenSizes"
import MainContext from "shared/context/mainContext"

import styles from "styles/shared.module.css"

export const AuthNavigator = (props: {
    link: string
    question?: string
    closer?: string
    onClick: () => void
    id?: string
}) => {
    const { width } = useContext(MainContext)

    const { question, link, closer, id, onClick } = props

    return (
        <div className="text-center">
            {question && (`${question} `)}
            {width < ScreenSize.SMALL && <br />}
            <span
                className="text-blue font-medium cursor-pointer"
                onClick={onClick}
                id={id}
            >
                {link}
            </span>
            {closer && closer}
        </div>
    )
}

export default AuthNavigator

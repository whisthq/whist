import React, { FC, useState, useEffect } from "react"

import { goTo } from "@app/utils/history"

interface FractalNavigationInterface {
    url: string
    text: string
    linkText?: string
}

const BaseNavigation: FC<FractalNavigationInterface> = (
    props: FractalNavigationInterface
) => {
    const [textList, setTextList] = useState([props.text])

    useEffect(() => {
        if (props.linkText) {
            if (!props.text.includes(props.linkText)) {
                throw new Error(
                    "prop linkText must be a substring of prop text"
                )
            } else {
                const textListTemp = props.text.split(props.linkText)
                setTextList(textListTemp)
            }
        }
    }, [props.linkText, props.text])

    return <>{textList.map((text: string) => {})}</>
}

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
            {beforeLink && `${beforeLink} `}
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

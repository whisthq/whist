import React, { FC, useState, useEffect } from "react"
import classNames from "classnames"

import { goTo } from "@app/utils/history"

interface BaseNavigationProps {
    url: string
    text: string
    linkText?: string
    className?: string
}

interface FractalNavigationProps extends BaseNavigationProps {}

const BaseNavigation: FC<BaseNavigationProps> = (
    props: BaseNavigationProps
) => {
    const [textList, setTextList] = useState([props.text])

    const onClick = (text: string) => {
        if (text === props.linkText) {
            goTo(props.url)
        }
    }

    useEffect(() => {
        if (props.linkText) {
            if (!props.text.includes(props.linkText)) {
                throw new Error(
                    "prop linkText must be a substring of prop text"
                )
            } else {
                const textListTemp = props.text.split(
                    new RegExp(`(${props.linkText})`)
                )
                setTextList(textListTemp)
            }
        }
    }, [props.linkText, props.text])

    return (
        <>
            {textList.map((text: string, index: number) => (
                <div key={`${index.toString()}`} className="inline">
                    <span
                        className={classNames(
                            text === props.linkText
                                ? "font-body text-blue font-medium cursor-pointer"
                                : "font-body font-medium",
                            props.className
                        )}
                        onClick={() => onClick(text)}
                    >
                        {text}
                    </span>
                </div>
            ))}
        </>
    )
}

export const FractalNavigation: FC<FractalNavigationProps> = (
    props: FractalNavigationProps
) => {
    return <BaseNavigation {...props} />
}

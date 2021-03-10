import React, { useContext } from "react"

import { ScreenSize } from "shared/constants/screenSizes"
import MainContext from "shared/context/mainContext"

import styles from "styles/shared.module.css"

export const SwitchMode = (props: {
    question: string
    link: string
    closer: string
    onClick: () => void
    id?: string
}) => {
    const { width } = useContext(MainContext)

    const { question, link, closer, id, onClick } = props

    return (
        <div className="font-body" style={{ textAlign: "center" }}>
            {question + " "}
            {width < ScreenSize.SMALL && <br />}
            <span
                style={{ color: "#3930b8" }}
                className={styles.hover}
                onClick={onClick}
                id={id}
            >
                {link}
            </span>
            {closer}
        </div>
    )
}

export default SwitchMode

import React, { useContext } from "react"

import { ScreenSize } from "shared/constants/screenSizes"
import MainContext from "shared/context/mainContext"

const SwitchMode = (props: {
    question: string
    link: string
    closer: string
    onClick: () => any
}) => {
    const { width } = useContext(MainContext)

    const { question, link, closer, onClick } = props

    return (
        <div style={{ textAlign: "center" }}>
            {question + " "}
            {width < ScreenSize.SMALL && <br />}
            <span
                style={{ color: "#3930b8" }}
                className="hover"
                onClick={onClick}
            >
                {link}
            </span>
            {closer}
        </div>
    )
}

export default SwitchMode

import React from "react"

const SwitchMode = (props: {
    question: string
    link: string
    closer: string
    onClick: () => any
    isFirstElement?: boolean
}) => {
    const { question, link, closer, onClick, isFirstElement } = props

    return (
        <div
            style={{ textAlign: "center", marginTop: isFirstElement ? 40 : 20 }}
        >
            {question + " "}
            <span
                style={{ color: "#3930b8" }}
                className="hover"
                onClick={onClick}
            >
                {link}
            </span>
            {" " + closer}
        </div>
    )
}

export default SwitchMode

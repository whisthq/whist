import React from "react"

const SwitchMode = (props: {
    question: string
    link: string
    closer: string
    onClick: () => any
}) => {
    const { question, link, closer, onClick } = props

    return (
        <div style={{ textAlign: "center" }}>
            {question + " "}
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

import React from "react"

import "shared/components/footer/components/pressCoverage/components/coverageBox.css"

export const CoverageBox = (props: {
    image: string
    text: string
    link: string
}) => {
    const { image, link } = props

    return (
        <div style={{ position: "relative" }}>
            <a href={link} target="_blank" rel="noopener noreferrer">
                <div className="boxWrapper">
                    <img src={image} className="boxImage" alt="coverage" />
                </div>
                {/* <div className="overlay">
                    <div className="overlayBox">{text}</div>
                </div> */}
            </a>
        </div>
    )
}

export default CoverageBox

import React from "react"

export const CoverageBox = (props: {
    image: string
    text: string
    link: string
}) => {
    const { image, link } = props

    return (
        <div style={{ position: "relative" }}>
            <a href={link} target="_blank" rel="noopener noreferrer">
                <div className="w-full h-full bg-white bg-opacity-10 rounded-lg">
                    <img src={image}
                         className="w-full max-h-60 rounded-lg"
                         style={{ filter: "grayscale(0.8)" }}
                         alt="coverage" />
                </div>
                {/* <div className="opacity-0 hover:opacity-1 hover:text-black hover:cursor-pointer absolute top-0 left-0 h-full transition duration-1000 text-base tracking-wide">
                    <div className="rounded-md w-full h-full px-1 py-1.5" bg-green-200>
                        {text}
                    </div>
                </div> */}
            </a>
        </div>
    )
}

export default CoverageBox

import React from "react"

import CountdownTimer from "pages/landing/components/countdown"

const Header = (props: any) => {
    return (
        <div
            style={{
                display: "flex",
                justifyContent: "space-between",
                width: "100%",
                padding: 30,
            }}
        >
            <div
                className="logo"
                style={{
                    marginBottom: 20,
                    color: props.color ? props.color : "white",
                }}
            >
                Fractal
            </div>
            <div>
                <CountdownTimer type="small" />
            </div>
        </div>
    )
}

export default Header

import React from "react"

import CountdownTimer from "pages/landing/components/countdown"

function Header(props: any) {
    return (
        <div
            style={{
                display: "flex",
                justifyContent: "space-between",
                width: "100%",
                padding: 30,
            }}
        >
            <div className="logo" style={{ marginBottom: 20 }}>
                Fractal
            </div>
            <div>
                <CountdownTimer type="small" />
            </div>
        </div>
    )
}

export default Header

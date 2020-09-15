import React from "react"

import "styles/landing.css"

function Testimonial(props: any) {
    return (
        <div
            style={{
                padding: "5px 30px",
                background: "rgba(172, 207, 231, 0.2)",
                borderRadius: 5,
                paddingLeft: 75,
                marginBottom: 25,
            }}
        >
            <div
                style={{
                    fontWeight: "bold",
                    fontSize: 125,
                    height: 120,
                }}
            >
                "
            </div>
            <p>{props.text}</p>
            <div
                style={{
                    fontWeight: "bold",
                    fontSize: 125,
                    textAlign: "right",
                    height: 100,
                    position: "relative",
                    bottom: 20,
                }}
            >
                "
            </div>
            <div style={{ position: "relative", bottom: 30 }}>
                <div style={{ fontWeight: "bold" }}>{props.title}</div>
                <div style={{ fontSize: 12 }}>{props.subtitle}</div>
            </div>
        </div>
    )
}

export default Testimonial

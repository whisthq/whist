import React from "react"
import { FaStar } from "react-icons/fa"

import "styles/landing.css"

const Testimonial = (props: any) => {
    return (
        <div
            style={{
                background: props.invert
                    ? "#3930b8"
                    : "rgba(213, 225, 245, 0.2)",
                borderRadius: 4,
            }}
        >
            <div
                style={{
                    marginTop: 10,
                    marginBottom: 20,
                    padding: "30px 45px",
                    color: props.invert ? "white" : "#111111",
                }}
            >
                <p>{props.text}</p>
                <div style={{ display: "flex" }}>
                    <FaStar
                        style={{
                            color: !props.invert ? "#3930b8" : "white",
                            marginRight: 5,
                        }}
                    />
                    <FaStar
                        style={{
                            color: !props.invert ? "#3930b8" : "white",
                            marginRight: 5,
                        }}
                    />
                    <FaStar
                        style={{
                            color: !props.invert ? "#3930b8" : "white",
                            marginRight: 5,
                        }}
                    />
                    <FaStar
                        style={{
                            color: !props.invert ? "#3930b8" : "white",
                            marginRight: 5,
                        }}
                    />
                    <FaStar
                        style={{
                            color: !props.invert ? "#3930b8" : "white",
                            marginRight: 5,
                        }}
                    />
                </div>
                <div style={{ marginTop: 20 }}>
                    <div style={{ fontWeight: "bold", fontSize: 15 }}>
                        {props.name}
                    </div>
                    <div style={{ fontSize: 14 }}>{props.job}</div>
                </div>
            </div>
        </div>
    )
}

export default Testimonial

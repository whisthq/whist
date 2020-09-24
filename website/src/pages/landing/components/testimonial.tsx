import React from "react"
import { FaStar } from "react-icons/fa"
import { useSpring, animated } from "react-spring"

import "styles/landing.css"

const calc = (x: number, y: number): any => {
    return [
        -(y - window.innerHeight / 2) / 20,
        (x - window.innerWidth / 2) / 20,
        1.05,
    ]
}

const trans = (x: any, y: any, s: any): any => {
    return `perspective(600px) rotateX(${x}deg) rotateY(${y}deg) scale(${s})`
}

function Testimonial(props: any) {
    const [springProps, set] = useSpring(() => ({
        xys: [0, 0, 1],
        config: { mass: 15, tension: 550, friction: 80 },
    }))

    return (
        <animated.div
            onMouseMove={({ clientX: x, clientY: y }) =>
                set({ xys: calc(x, y) })
            }
            onMouseLeave={() => set({ xys: [0, 0, 1] })}
            style={{
                background: props.invert
                    ? "#3930b8"
                    : "rgba(213, 225, 245, 0.2)",
                borderRadius: 4,
                transform: springProps.xys.interpolate(trans),
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
        </animated.div>
    )
}

export default Testimonial

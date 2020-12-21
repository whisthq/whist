import React, { useState, useContext } from "react"
import MainContext from "shared/context/mainContext"

const FloatingLogo = (props: any) => {
    const { appHighlight, setAppHighlight } = useContext(MainContext)
    const [hover, setHover] = useState(false)

    const calculateSize = () => {
        if (props.width > 720) {
            if (appHighlight) {
                if (appHighlight === props.app) {
                    return [175, 25]
                } else {
                    return [80, 13]
                }
            } else {
                return [100, 15]
            }
        } else {
            return [50, 7]
        }
    }

    const [size, padding] = calculateSize()

    return (
        <img
            src={
                props.currentIndex === props.textIndex ||
                hover ||
                props.currentIndex === 6
                    ? props.colorImage
                    : props.noColorImage
            }
            className="bounce"
            alt=""
            onClick={() => {
                if (props.app === appHighlight) {
                    setAppHighlight("")
                } else {
                    props.app && setAppHighlight(props.app)
                }
            }}
            style={{
                borderRadius: 4,
                background:
                    props.currentIndex === props.textIndex ||
                    hover ||
                    props.currentIndex === 6
                        ? "rgba(239,244,254,0.8)"
                        : "white",
                boxShadow:
                    props.currentIndex === props.textIndex ||
                    hover ||
                    props.currentIndex === 6
                        ? props.boxShadow
                        : "0px 4px 20px rgba(0,0,0,0.1)",
                padding: padding,
                width: size,
                height: size,
                position: "absolute",
                top: props.top,
                left: props.left,
                animationDelay: props.animationDelay,
            }}
            onMouseEnter={() => setHover(true)}
            onMouseLeave={() => setHover(false)}
        />
    )
}

export default FloatingLogo

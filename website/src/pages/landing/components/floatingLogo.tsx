import React, { useState, useContext } from "react"
import MainContext from "shared/context/mainContext"

function FloatingLogo(props: any) {
    const { appHighlight } = useContext(MainContext)
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
        <div>
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
                style={{
                    borderRadius: 4,
                    boxShadow:
                        props.currentIndex === props.textIndex ||
                        hover ||
                        props.currentIndex === 6
                            ? props.boxShadow
                            : "0px 4px 20px rgba(0,0,0,0.05)",
                    padding: padding,
                    width: size,
                    height: size,
                    position: "absolute",
                    zIndex: -1,
                    top: props.top,
                    left: props.left,
                    background:
                        props.currentIndex === props.textIndex ||
                        hover ||
                        props.currentIndex === 6
                            ? props.background
                            : "white",
                    animationDelay: props.animationDelay,
                }}
                onMouseEnter={() => setHover(true)}
                onMouseLeave={() => setHover(false)}
            />
        </div>
    )
}

export default FloatingLogo

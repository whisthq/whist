import React, { Children } from "react"
import { useTrail, animated } from "react-spring"

export const FractalFadeIn = (props: {
    children: JSX.Element | JSX.Element[]
}) => {
    const items = Children.toArray(props.children)
    const trail = useTrail(items.length, {
        from: { opacity: 0 },
        to: { opacity: 1 },
    })
    return (
        <div {...props}>
            <div>
                {trail.map((props: any, index: number) => (
                    <animated.div key={index.toString()} style={props}>
                        {items[index]}
                    </animated.div>
                ))}
            </div>
        </div>
    )
}

import React, { Children } from "react"
import { useTrail, animated } from "react-spring"

export const FractalFadeIn = (props: {
    children: JSX.Element | JSX.Element[]
}) => {
    /*
        Description:
            Wraps around children elements and applies a fade-in animation. If multiple
            children are present, each child is faded in sequentially

        Usage:
            <FractalFadeIn>
                <div>I will fade in</div>
                <div>I will fade in slightly later</div>
            </FractalFadeIn>

        Arguments:
            children(JSX.Element | JSX.Element[]): Any children nested within this component
    */

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

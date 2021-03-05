import React from "react"
import PuffLoader from "react-spinners/PuffLoader"

// in this page we'll have a bunch of small components that get reused to display loading animations
// (might change later to include other animations and visuals...)

/**
 * A puff loader that takes over the middle of the page.
 * @param props unused
 */
export const PuffAnimation = (props: {
    fullScreen?: boolean
}) => {
    const {fullScreen} = props

    const animationStyle = fullScreen ? "w-screen h-screen flex items-center justify-center" : "w-48 h-48 flex items-center justify-center m-auto"

    return(
        <div className={animationStyle}>
            <PuffLoader
            />
        </div>
    )
}


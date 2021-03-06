import React from "react"

import styles from "shared/components/loadingAnimation/loadingAnimation.css"

export const LoadingAnimation = () => {
    /*
        Animated Fractal logo SVG
 
        Arguments: none  
    */

    return (
        <div className={styles.animationWrapper}>
            <svg
                width="56"
                height="55"
                viewBox="0 0 56 55"
                fill="none"
                xmlns="http://www.w3.org/2000/svg"
            >
                <path
                    d="M37.25 4.21359L4 4.21359L4 37.466L20.75 37.466M20.75 37.466L20.75 21.2039H37.25M20.75 37.466V53L5.5 53L37.25 21.2039M37.25 21.2039L37.25 2.21274M37.25 21.2039L54 21.2039L54 5.18447L21 37.466H37.25V21.2039Z"
                    stroke="black"
                    strokeWidth="4"
                    className={styles.animatedLogo}
                />
            </svg>
        </div>
    )
}

export default LoadingAnimation

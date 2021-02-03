import React from "react"

import TypeWriterEffect from "react-typewriter-effect"

import styles from "pages/login/components/splashScreen/splashScreen.css"

export const SplashScreen = (props: { onClick: () => void }) => {
    /*
        Login splash screen with login button
 
        Arguments:
            onClick (function): callback function when user clicks login button 
    */

    const { onClick } = props

    return (
        <div style={{ textAlign: "left" }}>
            <div className={styles.loginText}>
                <div>A&nbsp;</div>
                <TypeWriterEffect
                    textStyle={{
                        fontSize: 80,
                        color: "#4f35de",
                    }}
                    startDelay={0}
                    cursorColor="#111111"
                    multiText={["faster", "lighter", "better"]}
                    loop
                    typeSpeed={200}
                />{" "}
                <div>&nbsp;browser&nbsp;</div>
            </div>
            <div
                style={{
                    fontSize: 25,
                    position: "relative",
                    bottom: 20,
                    fontWeight: "normal",
                }}
            >
                is one click away.
            </div>
            <button
                type="button"
                className={styles.transparentButton}
                onClick={onClick}
            >
                Login
            </button>
        </div>
    )
}

export default SplashScreen

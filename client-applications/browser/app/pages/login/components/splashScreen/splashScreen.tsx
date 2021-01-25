import React from "react"

import styles from "pages/login/components/splashScreen/splashScreen.css"

export const SplashScreen = (props: { onClick: () => void }) => {
    /*
        Login splash screen with login button
 
        Arguments:
            onClick (function): callback function when user clicks login button 
    */

    const { onClick } = props

    return (
        <div>
            <div className={styles.loginText}>
                A better browser is one click away.
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

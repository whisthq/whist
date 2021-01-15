import React, { ChangeEvent } from "react"

import styles from "pages/login/components/redirect/redirect.css"

export const Redirect = (props: {
    onClick: () => void
    onChange: (evt: ChangeEvent) => void
    onKeyPress: (evt: KeyboardEvent) => void
}) => {
    /*
        When the user clicks log in, they are redirected to this page, which tells
        them to check their browser.
 
        Arguments:
            onClick (function): Callback function to open browser URL again
            onChange (function): Callback function when user manually enters an access token (dev purposes only)
            onKeyPress (function): Callback function when user submits an access token (dev purposes only)
    */

    const { onClick, onChange, onKeyPress } = props

    return (
        <div>
            <div className={styles.loginText}>
                A browser window should open momentarily where you can login.
                Click{" "}
                <span
                    role="button"
                    tabIndex={0}
                    onClick={onClick}
                    onKeyDown={onClick}
                    style={{
                        fontWeight: "bold",
                        cursor: "pointer",
                    }}
                >
                    here
                </span>{" "}
                if it doesn&lsquo;t appear. Once you&lsquo;ve logged in, this
                page will automatically redirect.
            </div>
            {process.env.NODE_ENV === "development" && (
                <div style={{ marginTop: 40 }}>
                    <input
                        type="text"
                        onChange={onChange}
                        onKeyPress={onKeyPress}
                        className={styles.devInput}
                    />
                </div>
            )}
        </div>
    )
}

export default Redirect

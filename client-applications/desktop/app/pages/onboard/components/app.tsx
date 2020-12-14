import React from "react"
import { FaPlus, FaMinus } from "react-icons/fa"

import styles from "pages/onboard/onboard.css"

const App = (props: { app: any; selected: boolean }) => {
    const { app, selected } = props

    return (
        <div
            className={
                selected ? styles.appContainerSelected : styles.appContainer
            }
        >
            <img
                alt="App logo"
                src={app.logo_url}
                className={styles.appImage}
            />
            <div className={styles.plusButton}>
                {selected ? (
                    <FaMinus className={styles.faButton} />
                ) : (
                    <FaPlus className={styles.faButton} />
                )}
            </div>
        </div>
    )
}

export default App

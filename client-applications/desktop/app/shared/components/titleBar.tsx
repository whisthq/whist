import React from "react"
import Titlebar from "react-electron-titlebar"

import { OperatingSystem } from "shared/types/client"

import styles from "app.global.css"

const TitleBar = () => {
    const clientOS = require("os").platform()

    return (
        <>
            {clientOS === OperatingSystem.WINDOWS ? (
                <div>
                    <Titlebar />
                </div>
            ) : (
                <div className={styles.macTitleBar} />
            )}
        </>
    )
}

export default TitleBar

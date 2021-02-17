import React from "react"
import Titlebar from "react-electron-titlebar"

import { OperatingSystem } from "shared/types/client"

import "app.global.css"

const TitleBar = () => {
    const clientOS = require("os").platform()

    return (
        <>
            {clientOS === OperatingSystem.WINDOWS ? (
                <div>
                    <Titlebar />
                </div>
            ) : (
                <div className="macTitleBar" />
            )}
        </>
    )
}

export default TitleBar

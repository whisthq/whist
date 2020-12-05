import React, { useState, useEffect } from "react"

const Version = () => {
    const [version, setVersion] = useState("1.0.0")

    useEffect(() => {
        const appVersion = require("../../package.json").version
        setVersion(appVersion)
    }, [])

    return (
        <div
            style={{
                position: "fixed",
                bottom: 15,
                right: 15,
                fontSize: 11,
                color: "#D1D1D1",
            }}
        >
            Version: {version}
        </div>
    )
}

export default Version

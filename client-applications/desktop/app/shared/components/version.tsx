import React, { useState, useEffect } from "react"

import { FractalLogger } from "shared/utils/general/logging"

const Version = () => {
    const [version, setVersion] = useState("1.0.0")

    const logger = new FractalLogger()

    useEffect(() => {
        const appVersion = require("../../package.json").version
        setVersion(appVersion)
        logger.logInfo(`The version is ${appVersion}`)
    }, [])

    return (
        <>
            {version}
        </>
    )
}

export default Version

import React from "react"

import Header from "shared/components/header"

import "styles/shared.css"

const Support = (props: any) => {
    return (
        <div className="fractalContainer">
            <Header color="black" />
            <a
                href="mailto: support@fractalcomputers.com"
                className="header-link"
            >
                Support
            </a>
        </div>
    )
}

export default Support
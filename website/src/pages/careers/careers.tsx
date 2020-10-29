import React from "react"

import Header from "shared/components/header"

import "styles/shared.css"


const Careers = (props: any) => {
    return (
        <div className="fractalContainer">
            <Header color="black" />
            <a
                href="mailto: careers@fractalcomputers.com"
                className="header-link"
            >
                Careers
            </a>
        </div>
    )
}

export default Careers
import React, { useContext } from "react"
import { Link } from "react-router-dom"

import Countdown from "pages/landing/components/countdown"
import MainContext from "shared/context/mainContext"

function Header(props: any) {
    const { width } = useContext(MainContext)

    return (
        <div
            style={{
                display: "flex",
                justifyContent: "space-between",
                width: "100%",
                paddingBottom: 0,
                paddingTop: 25,
                borderBottom: "solid 1px #DFDFDF",
            }}
        >
            <div
                style={{
                    display: "flex",
                }}
            >
                <Link
                    to="/"
                    style={{
                        outline: "none",
                        textDecoration: "none",
                        marginRight: 100,
                    }}
                >
                    <div
                        className="logo"
                        style={{
                            marginBottom: 20,
                            color: props.color ? props.color : "white",
                        }}
                    >
                        Fractal
                    </div>
                </Link>
                {width > 720 ? (
                    <div style={{ display: "flex" }}>
                        <Link to="/auth" className="header-link">
                            My Account
                        </Link>
                        <Link to="/about" className="header-link">
                            About
                        </Link>
                        <a
                            href="mailto: support@ftryfractal.com"
                            className="header-link"
                        >
                            Support
                        </a>
                        <a
                            href="mailto: careers@tryfractal.com"
                            className="header-link"
                        >
                            Career
                        </a>
                    </div>
                ) : (
                    <div />
                )}
            </div>
            <div style={{ opacity: width > 720 ? 1.0 : 0.0 }}>
                <Countdown type="small" />
            </div>
        </div>
    )
}

export default Header

import React from "react"
import { Row, Col } from "react-bootstrap"

import Header from "shared/components/header"

import "styles/shared.css"

const Careers = (props: any) => {
    return (
        <div className="fractalContainer">
            <Header color="black" />
            <div
                style={{
                    width: 400,
                    margin: "auto",
                    marginTop: 70,
                    textAlign: "center",
                }}
            >
                <div
                    style={{
                        color: "#111111",
                        fontSize: 32,
                        fontWeight: "bold",
                    }}
                >
                    Positions Available
                </div>
                <Row>
                    <Col>a</Col>
                    <Col>a</Col>
                    <Col>a</Col>
                </Row>
                <a
                    href="mailto: careers@fractalcomputers.com"
                    className="header-link"
                >
                    Careers
                </a>
            </div>
        </div>
    )
}

export default Careers

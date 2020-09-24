import React, { useContext } from "react"
import { Row, Col } from "react-bootstrap"
import { HashLink } from "react-router-hash-link"
import {
    FaLinkedinIn,
    FaTwitter,
    FaFacebook,
    FaInstagram,
    FaMediumM,
} from "react-icons/fa"

import "styles/shared.css"

import MainContext from "shared/context/mainContext"

const Footer = (props: any) => {
    const { width } = useContext(MainContext)
    return (
        <div
            style={{ background: "#333333", color: "white", paddingBottom: 40 }}
        >
            <div
                className="footer"
                style={{
                    maxWidth: 1600,
                    margin: "auto",
                    paddingLeft: 50,
                    paddingRight: 50,
                }}
            >
                <div
                    style={{
                        display: width > 720 ? "flex" : "block",
                        justifyContent: "space-between",
                    }}
                >
                    <Row>
                        <Col xs={12} style={{ maxWidth: 350 }}>
                            <div className="title">Fractal</div>
                            <div className="text">
                                Fractal uses cloud streaming to supercharge your
                                laptop's laggy applications.
                            </div>
                            <div
                                style={{
                                    display: "flex",
                                    marginTop: 20,
                                    textAlign: "left",
                                }}
                            >
                                <a
                                    href="https://twitter.com/fractalcomputer"
                                    target="_blank"
                                    rel="noopener noreferrer"
                                    style={{
                                        textDecoration: "none",
                                    }}
                                >
                                    <div className="icon-box">
                                        <FaTwitter className="icon" />
                                    </div>
                                </a>
                                <a
                                    href="https://medium.com/@fractal"
                                    target="_blank"
                                    rel="noopener noreferrer"
                                    style={{
                                        textDecoration: "none",
                                    }}
                                >
                                    <div className="icon-box">
                                        <FaMediumM className="icon" />
                                    </div>
                                </a>
                                <a
                                    href="https://www.linkedin.com/company/fractalcomputers"
                                    target="_blank"
                                    rel="noopener noreferrer"
                                    style={{
                                        textDecoration: "none",
                                    }}
                                >
                                    <div className="icon-box">
                                        <FaLinkedinIn className="icon" />
                                    </div>
                                </a>
                                <a
                                    href="https://www.instagram.com/fractalcomputer/"
                                    target="_blank"
                                    rel="noopener noreferrer"
                                    style={{
                                        textDecoration: "none",
                                    }}
                                >
                                    <div className="icon-box">
                                        <FaInstagram className="icon" />
                                    </div>
                                </a>
                                <a
                                    href="https://www.facebook.com/fractalcomputer"
                                    target="_blank"
                                    rel="noopener noreferrer"
                                    style={{
                                        textDecoration: "none",
                                    }}
                                >
                                    <div className="icon-box">
                                        <FaFacebook className="icon" />
                                    </div>
                                </a>
                            </div>
                        </Col>
                    </Row>
                    <Row
                        style={{
                            width: width > 700 ? 300 : "100%",
                            textAlign: "left",
                            paddingTop: width > 720 ? 0 : 40,
                        }}
                    >
                        <Col
                            xs={6}
                            style={{
                                paddingTop: 15,
                                paddingBottom: 20,
                                textAlign: width > 720 ? "right" : "left",
                            }}
                        >
                            <div className="section-name">RESOURCES</div>
                            <div style={{ fontSize: 13, outline: "none" }}>
                                <div>
                                    <HashLink
                                        to="/about#top"
                                        className="page-link"
                                        style={{ outline: "none" }}
                                    >
                                        About
                                    </HashLink>
                                </div>
                            </div>
                            <div style={{ fontSize: 13, outline: "none" }}>
                                <div>
                                    <a
                                        href="https://medium.com/@fractal"
                                        className="page-link"
                                        target="_blank"
                                        rel="noopener noreferrer"
                                    >
                                        Blog
                                    </a>
                                </div>
                            </div>
                            <div style={{ fontSize: 13 }}>
                                <div>
                                    <a
                                        href="https://discord.gg/eG88g6k"
                                        className="page-link"
                                        target="_blank"
                                        rel="noopener noreferrer"
                                    >
                                        Discord
                                    </a>
                                </div>
                            </div>
                        </Col>
                        <Col
                            xs={6}
                            style={{
                                paddingTop: 15,
                                paddingBottom: 20,
                                textAlign: width > 720 ? "right" : "left",
                            }}
                        >
                            <div className="section-name">CONTACT</div>
                            <div>
                                <a
                                    href="mailto: sales@fractalcomputers.com"
                                    className="page-link"
                                >
                                    Sales
                                </a>
                            </div>
                            <div>
                                <a
                                    href="mailto: support@fractalcomputers.com"
                                    className="page-link"
                                >
                                    Support
                                </a>
                            </div>
                            <div>
                                <a
                                    href="mailto: careers@fractalcomputers.com"
                                    className="page-link"
                                >
                                    Careers
                                </a>
                            </div>
                        </Col>
                    </Row>
                </div>
                <div
                    className="fractal-container"
                    style={{
                        maxWidth: 1920,
                    }}
                >
                    <div
                        style={{
                            fontSize: 13,
                            marginTop: 20,
                            width: "100%",
                            display: "flex",
                            justifyContent: "space-between",
                        }}
                    >
                        <div
                            style={{
                                margin: 0,
                                color: "#cccccc",
                                overflow: "hidden",
                                fontSize: width > 700 ? 14 : 12,
                            }}
                        >
                            Copyright &copy; Fractal Computers, Inc. All Rights
                            Reserved.
                        </div>
                        {width > 700 && (
                            <div
                                style={{
                                    margin: 0,
                                    color: "#cccccc",
                                    overflow: "hidden",
                                }}
                            >
                                <HashLink
                                    to="/termsofservice#top"
                                    style={{ color: "#cccccc" }}
                                >
                                    Terms of Service
                                </HashLink>{" "}
                                &amp;{" "}
                                <HashLink
                                    to="/privacy#top"
                                    style={{ color: "#cccccc" }}
                                >
                                    Privacy Policy
                                </HashLink>
                            </div>
                        )}
                    </div>
                </div>
            </div>
        </div>
    )
}

export default Footer

import React, { useContext } from "react"
import { Row, Col } from "react-bootstrap"
import { HashLink } from "react-router-hash-link"
import { FaLinkedinIn, FaTwitter, FaInstagram, FaMediumM } from "react-icons/fa"

import styles from "styles/shared.module.css"

import { ScreenSize } from "shared/constants/screenSizes"
import MainContext from "shared/context/mainContext"
// import PressCoverage from "shared/components/footer/components/pressCoverage/pressCoverage"

const Footer = () => {
    const { width } = useContext(MainContext)
    return (
        <div
            style={{ background: "#060217", color: "white", paddingBottom: 40 }}
        >
            <div
                className={styles.footer}
                style={{
                    maxWidth: 1600,
                    margin: "auto",
                    paddingLeft: 50,
                    paddingRight: 50,
                }}
            >
                {/* <div
                    style={{
                        marginBottom: 50,
                        textAlign: "right",
                        width: "100%",
                    }}
                >
                    <PressCoverage />
                </div> */}
                <div
                    style={{
                        display: width >= ScreenSize.MEDIUM ? "flex" : "block",
                        justifyContent: "space-between",
                    }}
                >
                    <Row>
                        <Col xs={12} style={{ maxWidth: 350 }}>
                            <div className={styles.title}>Fractal</div>
                            <div className={styles.text}>
                                Fractal supercharges your applications by
                                streaming them from the cloud.
                            </div>
                            <div
                                style={{
                                    display: "flex",
                                    marginTop: 20,
                                    textAlign: "left",
                                }}
                            >
                                <a
                                    id="twitter"
                                    href="https://twitter.com/tryfractal"
                                    target="_blank"
                                    rel="noopener noreferrer"
                                    style={{
                                        textDecoration: "none",
                                    }}
                                >
                                    <div className={styles.iconBox}>
                                        <FaTwitter className={styles.icon} />
                                    </div>
                                </a>
                                <a
                                    id="medium"
                                    href="https://medium.com/@fractal"
                                    target="_blank"
                                    rel="noopener noreferrer"
                                    style={{
                                        textDecoration: "none",
                                    }}
                                >
                                    <div className={styles.iconBox}>
                                        <FaMediumM className={styles.icon} />
                                    </div>
                                </a>
                                <a
                                    id="linkedin"
                                    href="https://www.linkedin.com/company/fractal"
                                    target="_blank"
                                    rel="noopener noreferrer"
                                    style={{
                                        textDecoration: "none",
                                    }}
                                >
                                    <div className={styles.iconBox}>
                                        <FaLinkedinIn className={styles.icon} />
                                    </div>
                                </a>
                                <a
                                    id="instagram"
                                    href="https://www.instagram.com/tryfractal"
                                    target="_blank"
                                    rel="noopener noreferrer"
                                    style={{
                                        textDecoration: "none",
                                    }}
                                >
                                    <div className={styles.iconBox}>
                                        <FaInstagram className={styles.icon} />
                                    </div>
                                </a>
                            </div>
                        </Col>
                    </Row>
                    <Row
                        style={{
                            width: width >= ScreenSize.MEDIUM ? 300 : "100%",
                            textAlign: "left",
                            paddingTop: width >= ScreenSize.MEDIUM ? 0 : 40,
                        }}
                    >
                        <Col
                            xs={6}
                            style={{
                                paddingTop: 15,
                                paddingBottom: 20,
                                textAlign:
                                    width >= ScreenSize.MEDIUM
                                        ? "right"
                                        : "left",
                            }}
                        >
                            <div className={styles.sectionName}>RESOURCES</div>
                            <div style={{ fontSize: 13, outline: "none" }}>
                                <div>
                                    <HashLink
                                        to="/about#top"
                                        className={styles.pageLink}
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
                                        className={styles.pageLink}
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
                                        href="https://discord.gg/HjPpDGvEeA"
                                        className={styles.pageLink}
                                        target="_blank"
                                        rel="noopener noreferrer"
                                    >
                                        Join our Discord
                                    </a>
                                </div>
                            </div>
                        </Col>
                        <Col
                            xs={6}
                            style={{
                                paddingTop: 15,
                                paddingBottom: 20,
                                textAlign:
                                    width >= ScreenSize.MEDIUM
                                        ? "right"
                                        : "left",
                            }}
                        >
                            <div className={styles.sectionName}>CONTACT</div>
                            <div>
                                <a
                                    href="mailto: sales@fractal.co"
                                    className={styles.pageLink}
                                >
                                    Sales
                                </a>
                            </div>
                            <div>
                                <a
                                    href="mailto: support@fractal.co"
                                    className={styles.pageLink}
                                >
                                    Support
                                </a>
                            </div>
                            <div>
                                <a
                                    href="mailto: careers@fractal.co"
                                    className={styles.pageLink}
                                >
                                    Careers
                                </a>
                            </div>
                        </Col>
                    </Row>
                </div>
                <div
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
                                fontSize: width >= ScreenSize.MEDIUM ? 14 : 12,
                            }}
                        >
                            &copy; 2021 Fractal Computers, Inc. All Rights
                            Reserved.
                        </div>
                        {width >= ScreenSize.MEDIUM && (
                            <div
                                style={{
                                    margin: 0,
                                    color: "#cccccc",
                                    overflow: "hidden",
                                }}
                            >
                                <HashLink
                                    id="tosPage"
                                    to="/termsofservice#top"
                                    style={{ color: "#cccccc" }}
                                >
                                    Terms of Service
                                </HashLink>{" "}
                                &amp;{" "}
                                <HashLink
                                    id="privacyPage"
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

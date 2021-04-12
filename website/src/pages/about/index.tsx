import React, { useContext } from "react"
import { Row, Col, Carousel } from "react-bootstrap"

import InvestorBox from "./components/investorBox"
import EmployeeBox from "./components/employeeBox"

import Footer from "@app/shared/components/footer"
import Header from "@app/shared/components/header"
import SideBySide from "@app/shared/components/sideBySide"
import { ScreenSize } from "@app/shared/constants/screenSizes"
import MainContext from "@app/shared/context/mainContext"

import { teamData } from "./constants/team"

import sharedStyles from "@app/styles/global.module.css"

export const About = (props: { useWidth?: boolean }) => {
    const { useWidth } = props

    const { width } = useContext(MainContext)

    const screenWidth = useWidth === true || useWidth === false ? 992 : width

    let teamCards = []
    let shuffledTeamData = teamData
        .map((a) => ({ sort: Math.random(), value: a }))
        .sort((a, b) => a.sort - b.sort)
        .map((a) => a.value)
    for (var i = 0; i < shuffledTeamData.length; i += 3) {
        let teamGroup = shuffledTeamData.slice(i, i + 3)
        teamCards.push(
            <Carousel.Item key={i}>
                <Row>
                    {teamGroup.map((person, index) => (
                        <EmployeeBox
                            key={index}
                            image={person.image}
                            name={person.name}
                            text={person.text}
                        />
                    ))}
                </Row>
            </Carousel.Item>
        )
    }
    return (
        <div>
            <div
                style={{ paddingBottom: 150 }}
                id="top"
                className={sharedStyles.fractalContainer}
            >
                <div>
                    <Header dark={false} />
                </div>
                <div
                    style={{
                        marginTop: screenWidth > ScreenSize.LARGE ? 50 : 0,
                    }}
                >
                    <div>
                        <SideBySide case={"Gaming"} />
                    </div>
                </div>
                <Row>
                    <Col
                        md={12}
                        style={{
                            textAlign: "left",
                            marginTop:
                                screenWidth > ScreenSize.XLARGE ? 100 : 50,
                        }}
                    >
                        <h2>Our Stories</h2>
                        <p
                            className="font-body .heading"
                            style={{
                                marginTop: 25,
                            }}
                        >
                            We’re computer scientists passionate about the
                            future of personal computing. Here are a few of our
                            stories.
                        </p>
                    </Col>
                    <Col md={12} style={{ marginTop: 50, textAlign: "left" }}>
                        <div>
                            <Carousel controls={false}>{teamCards}</Carousel>
                        </div>
                    </Col>
                </Row>
                <div>
                    <Row style={{ marginTop: 100 }}>
                        <Col md={5}>
                            <h2>Our Investors</h2>
                            <p  className="font-body"
                                style={{
                                    marginTop: 30,
                                    marginBottom: 20,
                                }}
                            >
                                We’re fortunate to be backed by amazing
                                institutional and individual investors. We'd
                                also like to give special thanks to
                                <a
                                    target="__blank"
                                    href="https://hacklodge.org/"
                                    style={{
                                        textDecoration: "none",
                                        fontWeight: "bold",
                                        color: "#555555",
                                    }}
                                >
                                    &nbsp;Hack Lodge
                                </a>
                                , which connected us with some of our best
                                advisors and engineers.
                            </p>
                        </Col>
                        <Col
                            md={{
                                span: screenWidth > ScreenSize.LARGE ? 6 : 7,
                                offset: screenWidth > ScreenSize.LARGE ? 1 : 0,
                            }}
                            style={{ padding: 0 }}
                        >
                            <InvestorBox />
                        </Col>
                    </Row>
                </div>
            </div>
            <div>
                <Footer />
            </div>
        </div>
    )
}

export default About

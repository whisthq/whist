import React from "react"
import { Row, Col, Carousel } from "react-bootstrap"

import InvestorBox from "pages/homepage/about/components/investorBox"
import EmployeeBox from "pages/homepage/about/components/employeeBox"

import Footer from "shared/components/footer/footer"
import Header from "shared/components/header"
import SideBySide from "shared/components/sideBySide"

import { teamData } from "pages/homepage/about/constants/team"

import { HEADER, SIDE_BY_SIDE, ABOUT_IDS, FOOTER } from "testing/utils/testIDs"

import "./aboutCarousel.css"
import sharedStyles from "styles/shared.module.css"

export const About = (props: { useWidth?: boolean }) => {
    const { useWidth } = props
    // TODO: CHECK MOBILE COMPATIBILITY
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
                <div data-testid={HEADER}>
                    <Header dark={false} />
                </div>
                <div
                    style={{
                        marginTop: 50
                    }}
                >
                    <div data-testid={SIDE_BY_SIDE}>
                        <SideBySide case={"Gaming"} />
                    </div>
                </div>
                <Row>
                    <Col
                        md={12}
                        style={{
                            textAlign: "left",
                            marginTop: 50
                        }}
                    >
                        <h2>Our Stories</h2>
                        <p
                            className=".heading"
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
                        <div data-testid={ABOUT_IDS.TEAM}>
                            <Carousel controls={false}>{teamCards}</Carousel>
                        </div>
                    </Col>
                </Row>
                <div data-testid={ABOUT_IDS.INVESTORS}>
                    <Row style={{ marginTop: 100 }}>
                        <Col md={5}>
                            <h2>Our Investors</h2>
                            <p
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
                            md={6}
                            style={{ padding: 0 }}
                        >
                            <InvestorBox />
                        </Col>
                    </Row>
                </div>
            </div>
            <div data-testid={FOOTER}>
                <Footer />
            </div>
        </div>
    )
}

export default About

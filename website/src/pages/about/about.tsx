import React, { useContext } from "react"
import { Row, Col, Carousel } from "react-bootstrap"

import InvestorBox from "pages/about/components/investorBox"
import EmployeeBox from "pages/about/components/employeeBox"
import SideBySide from "shared/components/sideBySide"

import Header from "shared/components/header"
import Footer from "shared/components/footer"
import MainContext from "shared/context/mainContext"

import { teamData } from "pages/about/constants/team"

import "styles/about.css"

const About = (props: any) => {
    const { width } = useContext(MainContext)
    let teamCards = []
    let shuffledTeamData = teamData
        .map((a) => ({ sort: Math.random(), value: a }))
        .sort((a, b) => a.sort - b.sort)
        .map((a) => a.value)
    for (var i = 0; i < shuffledTeamData.length; i += 3) {
        let teamGroup = shuffledTeamData.slice(i, i + 3)
        teamCards.push(
            <Carousel.Item>
                <Row>
                    {teamGroup.map((person) => (
                        <EmployeeBox
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
                className="fractalContainer"
            >
                <Header dark={false} />
                <div style={{ marginTop: 50 }}>
                    <SideBySide case={"Gaming"} width={width} />
                </div>
                <Row>
                    <Col md={12} style={{ textAlign: "left", marginTop: 100 }}>
                        <h2>Our Stories</h2>
                        <p
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
                        <Carousel controls={false}>{teamCards}</Carousel>
                    </Col>
                </Row>
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
                            institutional and individual investors. We'd also
                            like to give special thanks to
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
                            , which connected us with some of our best advisors
                            and engineers.
                        </p>
                    </Col>
                    {width > 700 ? (
                        <Col md={7} style={{ paddingLeft: 60 }}>
                            <InvestorBox />
                        </Col>
                    ) : (
                        <Col md={7} style={{ padding: 0 }}>
                            <InvestorBox />
                        </Col>
                    )}
                </Row>
            </div>
            <Footer />
        </div>
    )
}

export default About

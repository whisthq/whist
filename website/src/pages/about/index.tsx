import React from 'react'
import { Row, Col, Carousel } from 'react-bootstrap'
import classNames from 'classnames'

import InvestorBox from './components/investorBox'
import EmployeeBox from './components/employeeBox'

import Footer from '@app/shared/components/footer'
import Header from '@app/shared/components/header'
import { ScreenSize } from '@app/shared/constants/screenSizes'
import { withContext } from '@app/shared/utils/context'

import { teamData } from './constants/team'

export const About = (props: { useWidth?: boolean }) => {
  const { useWidth } = props

  const { width } = withContext()

  const screenWidth = useWidth === true || useWidth === false ? 992 : width

  const teamCards = []
  const shuffledTeamData = teamData
    .map((a) => ({ sort: Math.random(), value: a }))
    .sort((a, b) => a.sort - b.sort)
    .map((a) => a.value)
  for (let i = 0; i < shuffledTeamData.length; i += 3) {
    const teamGroup = shuffledTeamData.slice(i, i + 3)
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

  const { dark } = withContext()

  return (
    <div
      className={classNames(
        'overflow-x-hidden',
        dark ? 'dark bg-blue-darkest' : 'bg-white'
      )}
    >
      <div
        style={{ paddingBottom: 150 }}
        id="top"
        className="pb-20 px-12 max-w-screen-2xl m-auto overflow-x-hidden"
      >
        <div>
          <Header dark={dark} />
        </div>
        <Row>
          <Col
            md={12}
            style={{
              textAlign: 'left',
              marginTop: screenWidth > ScreenSize.XLARGE ? 100 : 50
            }}
          >
            <h2 className="dark:text-gray-300">Our Stories</h2>
            <p
              className="font-body dark:text-gray-400"
              style={{
                marginTop: 25
              }}
            >
              We’re computer scientists passionate about the future of personal
              computing. Here are a few of our stories.
            </p>
          </Col>
          <Col md={12} style={{ marginTop: 50, textAlign: 'left' }}>
            <div>
              <Carousel controls={false}>{teamCards}</Carousel>
            </div>
          </Col>
        </Row>
        <div>
          <Row style={{ marginTop: 100 }}>
            <Col md={5}>
              <h2 className="dark:text-gray-300">Our Investors</h2>
              <p
                className="font-body dark:text-gray-400"
                style={{
                  marginTop: 30,
                  marginBottom: 20
                }}
              >
                We’re fortunate to be backed by amazing institutional and
                individual investors. We&lsquo;d also like to give special
                thanks to
                <a
                  target="__blank"
                  href="https://hacklodge.org/"
                  style={{
                    textDecoration: 'none',
                    fontWeight: 'bold',
                    color: '#555555'
                  }}
                >
                  &nbsp;Hack Lodge
                </a>
                , which connected us with some of our best advisors and
                engineers.
              </p>
            </Col>
            <Col
              md={{
                span: screenWidth > ScreenSize.LARGE ? 6 : 7,
                offset: screenWidth > ScreenSize.LARGE ? 1 : 0
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

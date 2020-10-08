import React, { useState } from 'react'
import { Row, Col, Carousel } from 'react-bootstrap'
import { connect } from 'react-redux'
import styles from 'styles/dashboard.css'

import { featuredAppData } from '../constants/featuredApps'

import App from '../components/app'

const Discover = (props: any) => {
    let featuredApps = []
    for (var i = 0; i < featuredAppData.length; i += 3) {
        let appGroup = featuredAppData.slice(i, i + 3)
        featuredApps.push(
            <Carousel.Item>
                <div className={styles.carouselRow}>
                    {appGroup.map((app) => (
                        <App app={app} />
                    ))}
                </div>
            </Carousel.Item>
        )
    }

    const [index, setIndex] = useState(0)
    const handleSelect = (selectedIndex: any) => {
        setIndex(selectedIndex)
    }

    return (
        <div className={styles.discover}>
            <Row style={{ padding: '20px' }}>
                <Col md={12} style={{ textAlign: 'left' }}>
                    <h2>Featured Apps</h2>
                </Col>
                <Col md={12} style={{ marginTop: 40 }}>
                    <Carousel
                        activeIndex={index}
                        onSelect={handleSelect}
                        indicators={false}
                        wrap={false}
                    >
                        {featuredApps}
                    </Carousel>
                </Col>
            </Row>
        </div>
    )
}

const mapStateToProps = (state: any) => {
    return {}
}

export default connect(mapStateToProps)(Discover)

import React, { useState } from 'react'
import { Row, Carousel } from 'react-bootstrap'
import { connect } from 'react-redux'
import styles from 'styles/dashboard.css'

import { featuredAppData } from '../constants/featuredApps'

import App from '../components/app'

const Discover = (props: any) => {
    return (
        <div className={styles.page}>
            <h2>Featured Apps</h2>
            <Row
                style={{
                    textAlign: 'left',
                    display: 'flex',
                    flexDirection: 'row',
                }}
            >
                {featuredAppData.map((app) => (
                    <App key={app.name} app={app} />
                ))}
            </Row>
        </div>
    )
}

const mapStateToProps = (state: any) => {
    return {}
}

export default connect(mapStateToProps)(Discover)

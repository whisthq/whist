import React from "react"
import { connect } from "react-redux"
import { Row, Col } from "react-bootstrap"

import Leaderboard from "pages/landing/components/leaderboard"
import Actions from "pages/landing/components/actions"

const LeaderboardView = (props: any) => {
    return (
        <div>
            <Row style={{ marginTop: 50 }}>
                <Col md={8}>
                    <Leaderboard />
                </Col>
                <Col md={4}>
                    <Actions />
                </Col>
            </Row>
        </div>
    )
}

export default connect()(LeaderboardView)

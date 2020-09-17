import React, { useContext } from "react"
import { connect } from "react-redux"
import { Row, Col } from "react-bootstrap"

import ScreenContext from "shared/context/screenContext"

import Mars from "assets/largeGraphics/mars.svg"
import Mercury from "assets/largeGraphics/mercury.svg"
import Mountain from "assets/largeGraphics/mountain.svg"

import Leaderboard from "pages/landing/components/leaderboard"
import Actions from "pages/landing/components/actions"

function LeaderboardView(props: any) {
    const { width } = useContext(ScreenContext)

    return (
        <div
            style={{
                padding: 30,
                marginTop: 30,
                marginBottom: 50,
                background: "#0f1726",
                paddingTop: 75,
                paddingBottom: 175,
                position: "relative",
            }}
        >
            <img
                src={Mars}
                alt=""
                style={{
                    position: "absolute",
                    width: 70,
                    height: 70,
                    top: 25,
                    right: -40,
                }}
            />
            <img
                src={Mercury}
                alt=""
                style={{
                    position: "absolute",
                    width: 60,
                    height: 60,
                    top: 475,
                    right: 90,
                }}
            />
            <img
                src={Mountain}
                alt=""
                style={{
                    position: "absolute",
                    width: "100vw",
                    bottom: 0,
                    left: 0,
                    zIndex: 1,
                    maxHeight: 150,
                }}
            />
            <div style={{ zIndex: 2, width: "100%" }}>
                <Row>
                    <Col
                        md={8}
                        style={{
                            paddingRight: width > 720 ? 40 : 0,
                            paddingLeft: 0,
                        }}
                    >
                        <Leaderboard />
                    </Col>
                    <Col md={4}>
                        <Actions />
                    </Col>
                </Row>
            </div>
        </div>
    )
}

export default connect()(LeaderboardView)

import React from "react"
import { Row, Col } from "react-bootstrap"

import Contrary from "assets/largeGraphics/contrary.svg"
import BostInno from "assets/largeGraphics/bostinno.svg"
import iLab from "assets/largeGraphics/ilab.svg"
import CoverageBox from "shared/components/footer/components/pressCoverage/components/coverageBox"

export const PressCoverage = () => {
    return (
        <div style={{ maxWidth: 220 }}>
            <div
                style={{
                    color: "#eeeeee",
                    textAlign: "left",
                    fontSize: 14,
                    marginBottom: 25,
                }}
            >
                <div
                    style={{
                        background: "rgba(255,255,255,0.1)",
                        display: "inline-block",
                        padding: "10px 25px",
                        borderRadius: 4,
                        letterSpacing: 1.4,
                    }}
                >
                    In the Press
                </div>
            </div>
            <Row>
                <Col md={4}>
                    <CoverageBox
                        image={Contrary}
                        text="Contrary Capital"
                        link="https://contrarycap.com/reports/2020-university-report.pdf"
                    />
                </Col>
                <Col md={4}>
                    <CoverageBox
                        image={BostInno}
                        text="BostInno"
                        link="https://www.bizjournals.com/boston/inno/stories/inno-insights/2020/09/24/25-under-25-young-entrepreneurs-boston-2020.html"
                    />
                </Col>
                <Col md={4}>
                    <CoverageBox
                        image={iLab}
                        text="Harvard iLab"
                        link="https://innovationlabs.harvard.edu/presidents-innovation-challenge/act2/"
                    />
                </Col>
            </Row>
        </div>
    )
}

export default PressCoverage

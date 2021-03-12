import React from "react"
import { Col } from "react-bootstrap"
import engineering from "@app/assets/icons/cloud-computing.svg"
import sales from "@app/assets/icons/newtons-cradle.svg"
import systems from "@app/assets/icons/gyroscope.svg"
import { HashLink } from "react-router-hash-link"
import { ScreenSize } from "@app/shared/constants/screenSizes"
import { debugLog } from "@app/shared/utils/logging"

const JobBox = (props: {
    width: number
    job: {
        role: string
        department: string
        location: string
        hours: string
        description: string
        responsibilities: string[]
        requirements: string[]
        niceToHaves: string[]
    }
}) => {
    let iconImg: string | undefined
    if (props.job.department === "engineering") {
        iconImg = engineering
    } else if (props.job.department === "sales") {
        iconImg = sales
    } else if (props.job.department === "systems") {
        iconImg = systems
    } else if (props.job.department == null) {
        debugLog("Error: Job department not set")
    } else {
        debugLog(
            "Error: job department incorrect. Must be engineering, sales, or systems"
        )
    }

    let jobUrl = "/careers?" + props.job.role.split(" ").join("-") + "#top"

    return (
        <Col md={4} style={{ marginBottom: 20 }}>
            <HashLink to={jobUrl} style={{ textDecoration: "none" }}>
                <div
                    className="expandOnHover"
                    style={{
                        background: "white",
                        borderRadius: 8,
                        boxShadow: "0px 4px 15px rgba(0, 0, 0, 0.15)",
                        padding: 30,
                    }}
                >
                    <img
                        src={iconImg}
                        alt=""
                        style={{
                            width: 50,
                            marginBottom: 30,
                        }}
                    />
                    <div
                        style={{
                            color: "#111111",
                            fontSize: 20,
                            fontWeight: "bold",
                        }}
                    >
                        {props.job.role}
                    </div>
                    <div
                        style={{
                            color: "#333333",
                            marginTop: 10,
                            fontSize:
                                props.width > ScreenSize.MEDIUM
                                    ? "calc(13px + 0.2vw)"
                                    : 14,
                        }}
                    >
                        {props.job.description}
                    </div>
                </div>
            </HashLink>
        </Col>
    )
}

export default JobBox

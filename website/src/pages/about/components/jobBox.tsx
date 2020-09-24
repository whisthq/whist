import React from "react"
import Col from "react-bootstrap/Col"
import engineering from "assets/icons/cloud-computing.svg"
import sales from "assets/icons/newtons-cradle.svg"
import systems from "assets/icons/gyroscope.svg"
import { HashLink } from "react-router-hash-link"

const JobBox = (props: any) => {
    let iconImg: any = null
    if (props.job.department === "engineering") {
        iconImg = engineering
    } else if (props.job.department === "sales") {
        iconImg = sales
    } else if (props.job.department === "systems") {
        iconImg = systems
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
                                props.width > 700 ? "calc(13px + 0.2vw)" : 14,
                        }}
                    >
                        {props.job.summary}
                    </div>
                </div>
            </HashLink>
        </Col>
    )
}

export default JobBox

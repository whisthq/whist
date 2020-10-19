import React, { useEffect } from "react"
import { Row } from "react-bootstrap"
import { connect } from "react-redux"
import { useQuery } from "@apollo/client"

import styles from "styles/dashboard.css"

import { GET_FEATURED_APPS } from "pages/constants/graphql"

import App from "../components/app"

const Discover = (props: any) => {
    const { data, error } = useQuery(GET_FEATURED_APPS)
    const featuredAppData = data ? data.hardware_supported_app_images : []

    useEffect(() => {
        console.log(error)
    }, [error])

    return (
        <div className={styles.page}>
            <h2>Featured Apps</h2>
            <Row
                style={{
                    textAlign: "left",
                    display: "flex",
                    flexDirection: "row",
                }}
            >
                {featuredAppData.map((app: any) => (
                    <App key={app.app_id} app={app} />
                ))}
            </Row>
        </div>
    )
}

const mapStateToProps = (state: any) => {
    return {}
}

export default connect(mapStateToProps)(Discover)

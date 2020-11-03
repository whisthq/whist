import React, { useEffect, useState } from "react"
import { Row, Carousel } from "react-bootstrap"
import { connect } from "react-redux"
import { useQuery } from "@apollo/client"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faAngleRight, faAngleLeft } from "@fortawesome/free-solid-svg-icons"
import { debugLog } from "shared/utils/logging"

import styles from "styles/dashboard.css"

import { GET_FEATURED_APPS } from "shared/constants/graphql"

import Banner from "pages/dashboard/components/banner"
import App from "pages/dashboard/components/app"

const Discover = (props: any) => {
    const { updateCurrentTab, search } = props

    const [searchResults, setSearchResults] = useState([])

    const checkActive = (app: any) => {
        return app.active
    }

    const getSearchResults = (app: any) => {
        return app.app_id.toLowerCase().startsWith(search.toLowerCase())
    }

    const { data } = useQuery(GET_FEATURED_APPS)
    const featuredAppData = data
        ? data.hardware_supported_app_images.filter(checkActive)
        : []

    useEffect(() => {
        const results = featuredAppData.filter(getSearchResults)
        setSearchResults(
            results.map((app: any) => <App key={app.app_id} app={app} />)
        )
    }, [search])

    let featuredApps = []
    for (var i = 0; i < featuredAppData.length; i += 3) {
        let appGroup = featuredAppData.slice(i, i + 3)
        featuredApps.push(
            <Carousel.Item style={{ width: "100%" }} key={i}>
                <Row
                    style={{
                        display: "flex",
                        flexDirection: "row",
                        alignItems: "flex-start",
                        margin: "50px 60px",
                    }}
                >
                    {appGroup.map((app: any) => (
                        <App key={app.app_id} app={app} />
                    ))}
                </Row>
            </Carousel.Item>
        )
    }

    const nextArrow = (
        <FontAwesomeIcon
            icon={faAngleRight}
            style={{
                color: "#999999",
                marginLeft: 50,
                height: 25,
            }}
        />
    )

    const prevArrow = (
        <FontAwesomeIcon
            icon={faAngleLeft}
            style={{
                color: "#999999",
                marginRight: 65,
                height: 25,
            }}
        />
    )

    return (
        <div style={{ flex: 1 }}>
            {search ? (
                <Row
                    style={{
                        display: "flex",
                        flexDirection: "row",
                        margin: "50px",
                    }}
                >
                    {searchResults.length > 0 ? (
                        searchResults
                    ) : (
                        <div
                            style={{
                                margin: "auto",
                                marginTop: "150px",
                                width: "500px",
                                textAlign: "center",
                            }}
                        >
                            It looks like we don't support that app yet! If it's
                            something you'd like to see, let us know through our{" "}
                            <span
                                className={styles.contactFormLink}
                                onClick={() => updateCurrentTab("Support")}
                            >
                                contact form.
                            </span>
                        </div>
                    )}
                </Row>
            ) : (
                <>
                    <Banner />
                    <Row
                        style={{
                            textAlign: "left",
                            display: "flex",
                            flexDirection: "row",
                        }}
                    >
                        <Carousel
                            controls
                            indicators={false}
                            interval={null}
                            wrap={false}
                            nextIcon={nextArrow}
                            prevIcon={prevArrow}
                            style={{
                                width: "100%",
                                position: "relative",
                                top: "-30px",
                                zIndex: 2,
                            }}
                        >
                            {featuredApps}
                        </Carousel>
                    </Row>
                </>
            )}
        </div>
    )
}

const mapStateToProps = () => {
    return {}
}

export default connect(mapStateToProps)(Discover)

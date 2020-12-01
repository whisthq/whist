import React, { useEffect, useState } from "react"
import { Row, Col, Carousel } from "react-bootstrap"
import { connect } from "react-redux"
import { useQuery } from "@apollo/client"

import { GET_FEATURED_APPS } from "shared/constants/graphql"
import { PuffAnimation } from "shared/components/loadingAnimations"

import App from "pages/dashboard/components/app"
import LeftColumn from "pages/dashboard/components/leftColumn"
import Banner from "pages/dashboard/components/banner"

const Discover = (props: any) => {
    const { updateCurrentTab, search } = props

    const [searchResults, setSearchResults] = useState([])
    const [selectedCategory, setSelectedCategory] = useState("All")

    const checkActive = (app: any) => {
        return app.active
    }

    const getSearchResults = (app: any) => {
        if (app && app.app_id && search) {
            return app.app_id.toLowerCase().startsWith(search.toLowerCase())
        }
    }

    const { data, loading } = useQuery(GET_FEATURED_APPS, {
        context: {
            headers: {
                Authorization: `Bearer ${props.accessToken}`,
            },
        },
    })
    const featuredAppData = data
        ? data.hardware_supported_app_images.filter(checkActive)
        : []

    const setCategory = (category: string): void => {
        setSelectedCategory(category)
    }

    useEffect(() => {
        const results = featuredAppData.filter(getSearchResults)
        setSearchResults(
            results.map((app: any) => <App key={app.app_id} app={app} />)
        )
    }, [search])

    let featuredApps = []
    featuredApps.push(
        <>
            {featuredAppData.map((app: any) => (
                <App key={app.app_id} app={app} />
            ))}
        </>
    )

    if (loading) {
        return (
            <div>
                <PuffAnimation />
            </div>
        )
    }
    return (
        <Row style={{ padding: "0px 50px", marginTop: 50 }}>
            <LeftColumn
                callback={setCategory}
                selectedCategory={selectedCategory}
            />
            <Col
                xs={11}
                style={{ overflowY: "scroll", height: 500, paddingBottom: 50 }}
            >
                <Row>
                    <Col
                        xs={8}
                        style={{
                            width: "100%",
                            height: 225,
                            paddingRight: 15,
                            borderRadius: 10,
                        }}
                    >
                        <Banner />
                    </Col>
                    <Col
                        xs={4}
                        style={{
                            width: "100%",
                            height: 225,
                        }}
                    >
                        <div
                            style={{
                                width: "100%",
                                height: "100%",
                                background: "white",
                                borderRadius: 10,
                            }}
                        ></div>
                    </Col>
                </Row>
                <Row style={{ marginTop: 15 }}>{featuredApps}</Row>
            </Col>
        </Row>
    )
}

const mapStateToProps = (state: any) => {
    return {
        accessToken: state.MainReducer.auth.accessToken,
    }
}

export default connect(mapStateToProps)(Discover)

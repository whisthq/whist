import React, { useEffect, useState } from "react"
import { Row, Col } from "react-bootstrap"
import { connect } from "react-redux"
import { useQuery } from "@apollo/client"

import { GET_FEATURED_APPS } from "shared/constants/graphql"
import { PuffAnimation } from "shared/components/loadingAnimations"
import { GET_BANNERS } from "shared/constants/graphql"
import App from "pages/dashboard/components/app"
import LeftColumn from "pages/dashboard/components/leftColumn"
import Banner from "pages/dashboard/components/banner"
import News from "pages/dashboard/components/news"

import FractalImg from "assets/images/fractal.svg"

/// here is a constant test app we add at the beginning
const adminApp = {
    app_id: "Test App",
    logo_url: FractalImg,
    category: "Test",
    description: "Test app for Fractal admins",
    long_description:
        "You can use the admin app to test if you are a Fractal admin (i.e. your email ends with @tryfractal). Go to settings and where the admin settings are set a task ARN, webserver (dev | local | staging | prod | a url), and region (us-east-1 | us-west-1 | ca-central-1). In any field you can enter reset to reset it to null. If you try to launch without setting then it will not work since it will be null (or your previous settings if you change one).",
    url: "tryfractal.com",
    tos: "https://www.tryfractal.com/termsofservice",
    active: true, // not used
}

const Discover = (props: any) => {
    const { accessToken, username, search } = props

    const adminUsername =
        username &&
        username.indexOf("@") > -1 &&
        username.split("@")[1] == "tryfractal.com"

    const [searchResults, setSearchResults] = useState([])
    const [selectedCategory, setSelectedCategory] = useState("All")
    const [featuredAppData, setFeaturedAppData] = useState([])

    const checkActive = (app: any) => {
        return app.active
    }

    const checkCategory = (app: any) => {
        if (selectedCategory === "All") {
            return true
        } else {
            return app.category === selectedCategory
        }
    }

    const getSearchResults = (app: any) => {
        if (app && app.app_id && search) {
            return app.app_id.toLowerCase().includes(search.toLowerCase())
        }
    }

    const appQuery = useQuery(GET_FEATURED_APPS, {
        context: {
            headers: {
                Authorization: `Bearer ${props.accessToken}`,
            },
        },
    })

    const bannerQuery = useQuery(GET_BANNERS, {
        context: {
            headers: {
                Authorization: `Bearer ${accessToken}`,
            },
        },
    })

    const bannerData = bannerQuery.data
        ? bannerQuery.data.hardware_banners.filter(
              (bannerData: any) => bannerData.category === "News"
          )
        : []

    const mediaData = bannerQuery.data
        ? bannerQuery.data.hardware_banners.filter(
              (bannerData: any) => bannerData.category === "Media"
          )
        : []

    const setCategory = (category: string): void => {
        setSelectedCategory(category)
    }

    useEffect(() => {
        const results = featuredAppData.filter(getSearchResults)
        setSearchResults(
            results.map((app: any) => (
                <App
                    key={app.app_id}
                    app={app}
                    admin={app.app_id === "Test App"}
                />
            ))
        )
    }, [search])

    useEffect(() => {
        if (appQuery.data) {
            var newAppData = appQuery.data
                ? appQuery.data.hardware_supported_app_images.filter(
                      checkActive
                  )
                : []
            if (selectedCategory) {
                newAppData = newAppData ? newAppData.filter(checkCategory) : []
            }
            // admins should get access to the test App
            if (adminUsername) {
                newAppData.push(adminApp)
            }

            setFeaturedAppData(newAppData)
        }
    }, [appQuery.data, selectedCategory])

    if (appQuery.loading || bannerQuery.loading) {
        return (
            <div>
                <PuffAnimation />
            </div>
        )
    } else if (search && searchResults.length > 0) {
        return (
            <Row style={{ padding: "0px 45px", marginTop: 25 }}>
                {searchResults}
            </Row>
        )
    } else {
        return (
            <div
                style={{
                    overflowX: "hidden",
                    overflowY: "scroll",
                    maxHeight: 525,
                    paddingBottom: 25,
                    marginTop: 25,
                }}
            >
                <Row style={{ padding: "0px 45px", marginTop: 20 }}>
                    <Col
                        xs={7}
                        style={{
                            width: "100%",
                            height: 225,
                            paddingRight: 15,
                            borderRadius: 10,
                        }}
                    >
                        <Banner bannerData={bannerData} />
                    </Col>
                    <News mediaData={mediaData} />
                </Row>
                <Row style={{ marginTop: 35, padding: "0px 45px" }}>
                    <LeftColumn
                        callback={setCategory}
                        selectedCategory={selectedCategory}
                    />
                    <Col xs={11}>
                        <Row>
                            {featuredAppData.map((app: any) => (
                                <App
                                    key={app.app_id}
                                    app={app}
                                    admin={app.app_id === "Test App"}
                                />
                            ))}
                        </Row>
                    </Col>
                </Row>
            </div>
        )
    }
}

const mapStateToProps = (state: any) => {
    return {
        username: state.MainReducer.auth.username,
        accessToken: state.MainReducer.auth.accessToken,
    }
}

export default connect(mapStateToProps)(Discover)

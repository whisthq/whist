import React, { useEffect, useState } from "react"
import { Row, Col } from "react-bootstrap"
import { connect } from "react-redux"
import { useQuery } from "@apollo/client"

import LeftColumn from "pages/dashboard/components/leftColumn/leftColumn"
import Banner from "pages/dashboard/components/banner/banner"
import News from "pages/dashboard/components/news/news"
import App from "pages/dashboard/components/app/app"

import { GET_FEATURED_APPS, GET_BANNERS } from "shared/constants/graphql"
import { PuffAnimation } from "shared/components/loadingAnimations"

import { FractalBannerCategory } from "shared/types/navigation"
import { FractalApp, FractalBanner } from "shared/types/ui"

import styles from "pages/dashboard/views/discover/discover.css"

import FractalImg from "assets/images/fractal.svg"

/// here is a constant test app we add at the beginning
const adminApp: any = {
    app_id: "Test App",
    logo_url: FractalImg,
    category: "Test",
    description: "Test app for Fractal admins",
    long_description:
        "You can use the admin app to test if you are a Fractal admin (i.e. your email ends with @tryfractal). Go to settings and where the admin settings are set a task ARN, webserver (dev | local | staging | prod | a url), and region (us-east-1 | us-west-1 | ca-central-1). In any field you can enter reset to reset it to null. If you try to launch without setting then it will not work since it will be null (or your previous settings if you change one). Cluster is null | string.",
    url: "tryfractal.com",
    tos: "https://www.tryfractal.com/termsofservice",
    active: true, // not used
}

const Discover = (props: { search: string }) => {
    const { search } = props

    const adminUsername =
        props.username &&
        props.username.indexOf("@") > -1 &&
        props.username.split("@")[1] == "tryfractal.com"

    // Define local state

    const [searchResults, setSearchResults] = useState([])
    const [selectedCategory, setSelectedCategory] = useState(
        FractalBannerCategory.ALL
    )
    const [featuredAppData, setFeaturedAppData] = useState([])

    // Helper functions to filter apps by category, active, search results

    const checkActive = (app: FractalApp) => {
        return app.active
    }

    const checkCategory = (app: FractalApp) => {
        if (selectedCategory === FractalBannerCategory.ALL) {
            return true
        }
        return app.category === selectedCategory
    }

    const getSearchResults = (app: FractalApp) => {
        if (app && app.app_id && search) {
            return app.app_id.toLowerCase().includes(search.toLowerCase())
        }
        return true
    }

    const setCategory = (category: string): void => {
        setSelectedCategory(category)
    }

    // GraphQL queries to get Fractal apps and banners

    const appQuery = useQuery(GET_FEATURED_APPS, {
        context: {
            headers: {
                Authorization: `Bearer ${accessToken}`,
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

    // Filter data queried from GraphQL above

    const bannerData = bannerQuery.data
        ? bannerQuery.data.hardware_banners.filter(
              (banner: FractalBanner) =>
                  banner.category === FractalBannerCategory.NEWS
          )
        : []

    const mediaData = bannerQuery.data
        ? bannerQuery.data.hardware_banners.filter(
              (banner: FractalBanner) =>
                  banner.category === FractalBannerCategory.MEDIA
          )
        : []

    // If user searches for an app, filter apps

    useEffect(() => {
        const results = featuredAppData.filter(getSearchResults)
        setSearchResults(
            results.map((app: FractalApp) => (
                <App
                    key={app.app_id}
                    app={app}
                    admin={app.app_id === "Test App"}
                />
            ))
        )
    }, [search])

    // If apps are queried via GraphQL, update local state and filter accordingly

    useEffect(() => {
        if (appQuery.data) {
            let newAppData = appQuery.data
                ? appQuery.data.hardware_supported_app_images.filter(
                      checkActive
                  )
                : []
            if (selectedCategory) {
                newAppData = newAppData ? newAppData.filter(checkCategory) : []
            }
            if (adminUsername) {
                newAppData.push(adminApp)
            }

            setFeaturedAppData(newAppData)
        }
    }, [appQuery.data, selectedCategory])

    // Display loading screen until GraphQL queries finish
    if (appQuery.loading || bannerQuery.loading) {
        return (
            <div>
                <PuffAnimation />
            </div>
        )
    }
    if (search && searchResults.length > 0) {
        return (
            <Row style={{ padding: "0px 45px", marginTop: 25 }}>
                {searchResults}
            </Row>
        )
    }
    return (
        <div className={styles.scrollWrapper}>
            <Row style={{ padding: "0px 45px", marginTop: 20 }}>
                <Col xs={7} className={styles.bannerWrapper}>
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
                        {featuredAppData.map((app: FractalApp) => (
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

const mapStateToProps = <T extends {}>(state: T): T => {
    return {
        accessToken: state.MainReducer.auth.accessToken,
        username: state.MainREducer.auth.username,
    }
}

export default connect(mapStateToProps)(Discover)

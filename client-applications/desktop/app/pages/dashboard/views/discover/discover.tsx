import React, { useEffect, useState } from "react"
import { Row, Col } from "react-bootstrap"
import { connect } from "react-redux"
import { useQuery } from "@apollo/client"

import LeftColumn from "pages/dashboard/components/leftColumn/leftColumn"
import Banner from "pages/dashboard/components/banner/banner"
import News from "pages/dashboard/components/news/news"
import App from "pages/dashboard/components/app/app"

import { GET_FEATURED_APPS } from "shared/constants/graphql"
import { PuffAnimation } from "shared/components/loadingAnimations"
import { GET_BANNERS } from "shared/constants/graphql"
import { FractalBannerCategory } from "shared/enums/navigation"
import { FractalApp, FractalBanner } from "shared/types/ui"

import styles from "pages/dashboard/views/discover/discover.css"

const Discover = (props: { search: string }) => {
    const { search } = props

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
        } else {
            return app.category === selectedCategory
        }
    }

    const getSearchResults = (app: FractalApp) => {
        if (app && app.app_id && search) {
            return app.app_id.toLowerCase().includes(search.toLowerCase())
        }
    }

    const setCategory = (category: string): void => {
        setSelectedCategory(category)
    }

    // GraphQL queries to get Fractal apps and banners

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
                Authorization: `Bearer ${props.accessToken}`,
            },
        },
    })

    // Filter data queried from GraphQL above

    const bannerData = bannerQuery.data
        ? bannerQuery.data.hardware_banners.filter(
              (bannerData: FractalBanner) =>
                  bannerData.category === FractalBannerCategory.NEWS
          )
        : []

    const mediaData = bannerQuery.data
        ? bannerQuery.data.hardware_banners.filter(
              (bannerData: FractalBanner) =>
                  bannerData.category === FractalBannerCategory.MEDIA
          )
        : []

    // If user searches for an app, filter apps

    useEffect(() => {
        const results = featuredAppData.filter(getSearchResults)
        setSearchResults(
            results.map((app: FractalApp) => <App key={app.app_id} app={app} />)
        )
    }, [search])

    // If apps are queried via GraphQL, update local state and filter accordingly

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
    } else if (search && searchResults.length > 0) {
        return (
            <Row style={{ padding: "0px 45px", marginTop: 25 }}>
                {searchResults}
            </Row>
        )
    } else {
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
                                <App key={app.app_id} app={app} />
                            ))}
                        </Row>
                    </Col>
                </Row>
            </div>
        )
    }
}

const mapStateToProps = <T extends {}>(state: T): T => {
    return {
        accessToken: state.MainReducer.auth.accessToken,
    }
}

export default connect(mapStateToProps)(Discover)

import React, { useEffect, useState } from "react"
import { Row, Col } from "react-bootstrap"
import { connect } from "react-redux"
import { useQuery } from "@apollo/client"

import LeftColumn from "pages/dashboard/components/leftColumn/leftColumn"
import Banner from "pages/dashboard/components/banner/banner"
import Media from "pages/dashboard/components/media/media"
import App from "pages/dashboard/components/app/app"

import { GET_FEATURED_APPS, GET_BANNERS } from "shared/constants/graphql"
import { PuffAnimation } from "shared/components/loadingAnimations"
import {
    checkIfShortcutExists,
    createShortcutName,
} from "shared/utils/shortcuts"
import { FractalApp, FractalBanner } from "shared/types/ui"
import { deep_copy } from "shared/utils/reducerHelpers"

import {
    FractalAppCategory,
    FractalBannerCategory,
} from "shared/types/navigation"

import styles from "pages/dashboard/views/discover/discover.css"

import { adminApp } from "pages/dashboard/constants/adminApp"

const Discover = (props: {
    search: string
    username: string
    accessToken: string
}) => {
    const { search, username, accessToken } = props

    const adminUsername =
        username &&
        username.indexOf("@") > -1 &&
        username.split("@")[1] === "tryfractal.com"

    // Define local state

    const [searchResults, setSearchResults] = useState([])
    const [selectedCategory, setSelectedCategory] = useState(
        FractalAppCategory.ALL
    )
    // For app cards
    const [featuredAppData, setFeaturedAppData] = useState([])
    // For left-side banner
    const [bannerData, setBannerData] = useState([])
    // For right-side banner with media articles
    const [mediaData, setMediaData] = useState([])

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

    // Helper functions to filter apps by category, active, search results

    const checkActive = (app: FractalApp) => {
        return app.active
    }

    const checkCategory = (app: FractalApp) => {
        if (selectedCategory === FractalAppCategory.ALL) {
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

    const setCategory = (category: FractalAppCategory): void => {
        setSelectedCategory(category)
    }

    // Filter data queried from GraphQL above

    useEffect(() => {
        if (bannerQuery && bannerQuery.data) {
            setBannerData(
                bannerQuery.data.hardware_banners.filter(
                    (banner: FractalBanner) =>
                        banner.category === FractalBannerCategory.NEWS
                )
            )
            setMediaData(
                bannerQuery.data.hardware_banners.filter(
                    (banner: FractalBanner) =>
                        banner.category === FractalBannerCategory.MEDIA
                )
            )
        }
    }, [bannerQuery])

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
        if (appQuery.data && appQuery.data.hardware_supported_app_images) {
            const supportedImages = appQuery.data.hardware_supported_app_images
            let localAppData: FractalApp[] = []
            for (let i = 0; i < supportedImages.length; i += 1) {
                let app: FractalApp = supportedImages[i]
                if (!checkActive(app)) {
                    continue
                }
                if (!checkCategory(app)) {
                    continue
                }
                const shortcutName = createShortcutName(app.app_id)
                const appCopy = Object.assign(deep_copy(app), {
                    installed: checkIfShortcutExists(shortcutName),
                })
                localAppData.push(appCopy)
            }
            if (adminUsername) {
                localAppData.push(adminApp)
            }

            setFeaturedAppData(localAppData)
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
                <Media mediaData={mediaData} />
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

const mapStateToProps = (state: {
    MainReducer: {
        auth: {
            accessToken: string
            username: string
        }
    }
}) => {
    return {
        accessToken: state.MainReducer.auth.accessToken,
        username: state.MainReducer.auth.username,
    }
}

export default connect(mapStateToProps)(Discover)

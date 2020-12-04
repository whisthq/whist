import React, { useEffect, useState, Dispatch } from "react"
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
import {
    FractalApp,
    FractalBanner,
    FractalAppLocalState,
} from "shared/types/ui"
import { deep_copy } from "shared/utils/reducerHelpers"
import { updateClient } from "store/actions/pure"
import { searchArrayByKey } from "shared/utils/helpers"
import {
    FractalAppCategory,
    FractalBannerCategory,
} from "shared/types/navigation"

import styles from "pages/dashboard/views/discover/discover.css"

const Discover = (props: {
    search: string
    apps: FractalApp
    username: string
    accessToken: string
    dispatch: Dispatch
}) => {
    const { search, username, apps, accessToken, dispatch } = props

    const adminUsername =
        username &&
        username.indexOf("@") > -1 &&
        username.split("@")[1] === "tryfractal.com"

    // Define local state

    const [searchResults, setSearchResults] = useState([])
    const [selectedCategory, setSelectedCategory] = useState(
        FractalAppCategory.ALL
    )

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
        const results = apps.filter(getSearchResults)
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
        if (
            apps &&
            apps.length === 0 &&
            appQuery.data &&
            appQuery.data.hardware_supported_app_images &&
            appQuery.data.hardware_supported_app_images.length > 0
        ) {
            const supportedImages = appQuery.data.hardware_supported_app_images
            let localAppData: FractalApp[] = []
            for (let i = 0; i < supportedImages.length; i += 1) {
                let app: FractalApp = supportedImages[i]
                // If the app is not ready for release, don't store or display it
                if (!checkActive(app)) {
                    continue
                }
                // Check to see if the app is already installed
                const shortcutName = createShortcutName(app.app_id)
                const installed = checkIfShortcutExists(shortcutName)
                // Check to see if the app is already in Redux state
                const { value } = searchArrayByKey(apps, "app_id", app.app_id)
                // Set the app state to INSTALLED, NOT_INSTALLED, INSTALLING, or DELETING
                let localState = installed
                    ? FractalAppLocalState.INSTALLED
                    : FractalAppLocalState.NOT_INSTALLED

                if (value) {
                    localState = value.localState
                }
                // Push app to app array
                const appCopy = Object.assign(deep_copy(app), {
                    localState: localState,
                })
                localAppData.push(appCopy)
            }

            if (adminUsername) {
                localAppData.push(adminApp)
            }

            dispatch(updateClient({ apps: localAppData }))
        }
    }, [appQuery.data, apps])

    // Display loading screen until GraphQL queries finish
    if (appQuery.loading || bannerQuery.loading) {
        return (
            <div>
                <PuffAnimation />
            </div>
        )
        // Display search results if the user searches
    } else if (search && searchResults.length > 0) {
        return (
            <Row style={{ padding: "0px 45px", marginTop: 25 }}>
                {searchResults}
            </Row>
        )
    }
    // Display the normal App Store page
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
                        {apps.filter(checkCategory).map((app: FractalApp) => (
                            <App key={app.app_id} app={app} />
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
        client: {
            apps: FractalApp[]
        }
    }
}) => {
    return {
        accessToken: state.MainReducer.auth.accessToken,
        username: state.MainReducer.auth.username,
        apps: state.MainReducer.client.apps,
    }
}

export default connect(mapStateToProps)(Discover)

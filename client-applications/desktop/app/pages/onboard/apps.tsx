import React, { useState, useEffect, Dispatch } from "react"
import { connect } from "react-redux"
import styles from "pages/onboard/onboard.css"
import { useQuery } from "@apollo/client"
import PuffLoader from "react-spinners/PuffLoader"

import Version from "shared/components/version"
import TitleBar from "shared/components/titleBar"
import App from "pages/onboard/components/app"

import { GET_FEATURED_APPS } from "shared/constants/graphql"
import { FractalRoute } from "shared/types/navigation"
import { history } from "store/history"
import { FractalAuthCache } from "shared/types/cache"
import { updateClient } from "store/actions/pure"
import { FractalApp } from "shared/types/ui"
import { searchArrayByKey } from "shared/utils/general/helpers"

const { shell } = require("electron")

const Apps = (props: { accessToken: string; dispatch: Dispatch<any> }) => {
    const { accessToken, dispatch } = props

    const [featuredAppData, setFeaturedAppData] = useState([])
    const [selectedApps, setSelectedApps] = useState<FractalApp[]>([])
    const [numSelectedApps, setNumSelectedApps] = useState(0)

    const checkActive = (app: FractalApp): boolean => {
        return app.active
    }

    const getAppIndex = (app: FractalApp): number => {
        const { index } = searchArrayByKey(selectedApps, "app_id", app.app_id)
        return index
    }

    const updateSelectedApps = (app: FractalApp): void => {
        const newSelectedApps: FractalApp[] = Object.assign([], selectedApps)
        const index = getAppIndex(app)
        if (index > -1) {
            newSelectedApps.splice(index, 1)
            setNumSelectedApps(numSelectedApps - 1)
        } else {
            newSelectedApps.push(app)
            setNumSelectedApps(numSelectedApps + 1)
        }
        setSelectedApps(newSelectedApps)
    }

    // GraphQL queries to get Fractal apps
    const { loading, data } = useQuery(GET_FEATURED_APPS, {
        context: {
            headers: {
                Authorization: `Bearer ${accessToken}`,
            },
        },
    })

    const handleDownload = () => {
        const Store = require("electron-store")
        const storage = new Store()
        storage.set(FractalAuthCache.ONBOARDED, true)

        if (numSelectedApps === 0) {
            history.push(FractalRoute.DASHBOARD)
        } else {
            dispatch(updateClient({ onboardApps: selectedApps }))
            history.push(FractalRoute.ONBOARD_INSTALLING)
        }
    }

    useEffect(() => {
        if (data) {
            const newAppData = data
                ? data.hardware_supported_app_images.filter(checkActive)
                : []
            setFeaturedAppData(newAppData)
        }
    }, [data])

    return (
        <div className={styles.scrollableContainer} data-tid="container">
            <Version />
            <TitleBar />
            <div className={styles.removeDrag}>
                <div className={styles.appsContainer}>
                    <h2 className={styles.appsTitle}>
                        Which apps do you want to run faster?
                    </h2>
                    <div className={styles.subtext}>
                        Fractal apps speed up your computer by running on cloud
                        hardware.
                    </div>
                    <div className={styles.appsWrapper}>
                        {loading ? (
                            <PuffLoader
                                css="margin-top: 75px !important; margin-bottom: 75px !important; margin: auto;"
                                size={75}
                            />
                        ) : (
                            <>
                                {featuredAppData.map((app: FractalApp) => (
                                    <button
                                        type="button"
                                        key={app.app_id}
                                        style={{
                                            margin: "20px 20px",
                                            padding: 0,
                                        }}
                                        onClick={() => updateSelectedApps(app)}
                                    >
                                        <App
                                            app={app}
                                            selected={getAppIndex(app) > -1}
                                        />
                                    </button>
                                ))}
                            </>
                        )}
                    </div>
                    <button
                        type="button"
                        className={styles.enterButton}
                        onClick={handleDownload}
                    >
                        DOWNLOAD {numSelectedApps} APPS
                    </button>
                    <div className={styles.bottomText}>
                        Don&apos;t see your{" "}
                        <button
                            type="button"
                            className={styles.link}
                            onClick={() =>
                                shell.openExternal(
                                    "https://tryfractal.typeform.com/to/AdCZ8ad2"
                                )
                            }
                        >
                            favorite app
                        </button>
                        ?
                    </div>
                </div>
            </div>
        </div>
    )
}

export const mapStateToProps = (state: {
    MainReducer: {
        auth: {
            accessToken: string
        }
    }
}) => {
    return {
        accessToken: state.MainReducer.auth.accessToken,
    }
}

export default connect(mapStateToProps)(Apps)

import React, { useState } from "react"

import TitleBar from "shared/components/titleBar"
import NavBar from "pages/dashboard/components/navBar/navBar"
import Discover from "pages/dashboard/views/discover/discover"
import Settings from "pages/dashboard/views/settings/settings"
import Support from "pages/dashboard/views/support/support"

import { FractalDashboardTab } from "shared/types/navigation"

import styles from "pages/dashboard/dashboard.css"

const Dashboard = () => {
    const [currentTab, setCurrentTab] = useState(FractalDashboardTab.APP_STORE)
    const [search, setSearch] = useState("")

    return (
        <div className={styles.container}>
            <TitleBar />
            <div
                style={{
                    display: "flex",
                    flexDirection: "column",
                    marginTop: 28,
                }}
                className={styles.removeDrag}
            >
                <NavBar
                    updateCurrentTab={setCurrentTab}
                    currentTab={currentTab}
                    search={search}
                    updateSearch={setSearch}
                />
                {currentTab === FractalDashboardTab.APP_STORE && (
                    <Discover search={search} />
                )}
                {currentTab === FractalDashboardTab.SETTINGS && <Settings />}
                {currentTab === FractalDashboardTab.SUPPORT && <Support />}
            </div>
        </div>
    )
}

export default Dashboard

import React, { useState } from "react"
import { connect } from "react-redux"
import Titlebar from "react-electron-titlebar"

import NavBar from "pages/dashboard/components/navBar/navBar"
import Discover from "pages/dashboard/views/discover/discover"
import Settings from "pages/dashboard/views/settings/settings"
import Support from "pages/dashboard/views/support/support"

import { FractalDashboardTab } from "shared/enums/navigation"
import { OperatingSystem } from "shared/enums/client"

import styles from "pages/dashboard/dashboard.css"

const Dashboard = (props: { os: string }) => {
    const { os } = props

    const [currentTab, setCurrentTab] = useState(FractalDashboardTab.APP_STORE)
    const [search, setSearch] = useState("")

    return (
        <div className={styles.container}>
            {os === OperatingSystem.WINDOWS ? (
                <div className={styles.titleBar}>
                    <Titlebar backgroundColor="#000000" />
                </div>
            ) : (
                <div className={styles.macTitleBar} />
            )}
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

const mapStateToProps = <T extends {}>(state: T): T => {
    return {
        os: state.MainReducer.client.os,
    }
}

export default connect(mapStateToProps)(Dashboard)

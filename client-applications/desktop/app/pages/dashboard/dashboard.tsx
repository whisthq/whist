import React, { useState } from "react"
import { connect } from "react-redux"
import styles from "styles/dashboard.css"
import Titlebar from "react-electron-titlebar"

import NavBar from "pages/dashboard/components/navBar"
import Discover from "pages/dashboard/views/discover"
import Settings from "pages/dashboard/views/settings"
import Support from "pages/dashboard/views/support"

const Dashboard = (props: any) => {
    const { os } = props
    const [currentTab, setCurrentTab] = useState("App Store")
    const [search, setSearch] = useState("")

    return (
        <div className={styles.container}>
            {os === "win32" ? (
                <div
                    style={{
                        zIndex: 9999,
                        position: "fixed",
                        top: 0,
                        right: 0,
                    }}
                >
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
                {currentTab === "App Store" && <Discover />}
                {currentTab === "Settings" && <Settings />}
                {currentTab === "Support" && <Support />}
            </div>
        </div>
    )
}

const mapStateToProps = (state: any) => {
    return {
        username: state.MainReducer.auth.username,
        os: state.MainReducer.client.os,
        launchURL: state.MainReducer.container.launchURL,
    }
}

export default connect(mapStateToProps)(Dashboard)

import React, { useState } from "react"
import { connect } from "react-redux"
import styles from "styles/dashboard.css"
import Titlebar from "react-electron-titlebar"

import NavBar from "pages/dashboard/components/navBar"
import Discover from "pages/dashboard/views/discover"
import Settings from "pages/dashboard/views/settings"
import Support from "pages/dashboard/views/support"

const Dashboard = (props: any) => {
    const { dispatch, username, os } = props
    const [currentTab, setCurrentTab] = useState("Discover")

    return (
        <div className={styles.container}>
            {os === "win32" ? (
                <div>
                    <Titlebar backgroundColor="#000000" />
                </div>
            ) : (
                <div className={styles.macTitleBar} />
            )}
            <div
                style={{ display: "flex", flexDirection: "column" }}
                className={styles.removeDrag}
            >
                <NavBar updateCurrentTab={setCurrentTab} />
                {currentTab === "Discover" && <Discover />}
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
    }
}

export default connect(mapStateToProps)(Dashboard)

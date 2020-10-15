import React, { useState } from "react"
import { connect } from "react-redux"
import styles from "styles/dashboard.css"
import Titlebar from "react-electron-titlebar"

import NavBar from "./components/navBar"
import Discover from "./pages/discover"
import Settings from "./pages/settings"
import Support from "./pages/support"

const Dashboard = (props: any) => {
    const { dispatch, username, os } = props
    const [currentTab, setCurrentTab] = useState("Discover")

    return (
        <div className={styles.container} style={{ background: "#f2f4fc" }}>
            {os === "win32" ? (
                <div>
                    <Titlebar backgroundColor="#000000" />
                </div>
            ) : (
                <div className={styles.macTitleBar} />
            )}
            <div
                style={{ display: "flex", flexDirection: "row" }}
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

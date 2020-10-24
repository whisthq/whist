import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { Collapse } from "react-bootstrap"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faUser } from "@fortawesome/free-solid-svg-icons"
import { history } from "store/configureStore"
import styles from "styles/dashboard.css"

import { resetState } from "store/actions/pure"

const NavTitle = (props: {
    selected: boolean
    text: string
    onClick: any
    style?: any
}) => {
    const { selected, text, onClick, style } = props
    return (
        <div
            className={selected ? styles.selectedNavTitle : styles.navTitle}
            onClick={onClick}
            style={style}
        >
            {text}
        </div>
    )
}

const NavBar = (props: any) => {
    const { dispatch, updateCurrentTab, username, name, currentTab } = props
    const [showProfile, setShowProfile] = useState(false)

    const updateTab = (tab: string) => {
        updateCurrentTab(tab)
    }

    const handleSignout = () => {
        const storage = require("electron-json-storage")
        storage.set("credentials", { username: "", password: "" })
        dispatch(resetState())
        history.push("/")
    }

    return (
        <div className={styles.navBar}>
            <div className={styles.logoTitle}>Fractal</div>
            <NavTitle
                selected={currentTab == "Discover"}
                text="Discover"
                onClick={() => updateTab("Discover")}
                style={{
                    marginLeft: 50,
                }}
            />
            <NavTitle
                selected={currentTab == "Settings"}
                text="Settings"
                onClick={() => updateTab("Settings")}
            />
            <NavTitle
                selected={currentTab == "Support"}
                text="Support"
                onClick={() => updateTab("Support")}
            />
            <div
                style={{
                    position: "absolute",
                    right: "50px",
                    top: "15px",
                    height: "max-content",
                }}
            >
                <div
                    className={styles.userInfo}
                    onClick={() => setShowProfile(!showProfile)}
                >
                    <FontAwesomeIcon
                        icon={faUser}
                        style={{
                            color: "#555555",
                            fontSize: 35,
                            padding: 6,
                            marginRight: 4,
                        }}
                    />
                    <div style={{ display: "flex", flexDirection: "column" }}>
                        <span className={styles.name}>{name}</span>
                        <span className={styles.email}>{username}</span>
                    </div>
                </div>
                <Collapse in={showProfile}>
                    <div onClick={handleSignout}>
                        <div className={styles.signoutButton}>SIGN OUT</div>
                    </div>
                </Collapse>
            </div>
        </div>
    )
}

const mapStateToProps = (state: any) => {
    return {
        username: state.MainReducer.auth.username,
        name: state.MainReducer.auth.name,
    }
}

export default connect(mapStateToProps)(NavBar)

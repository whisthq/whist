import React, { useState } from "react"
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
    const {
        dispatch,
        updateCurrentTab,
        username,
        name,
        currentTab,
        search,
        updateSearch,
    } = props
    const [showProfile, setShowProfile] = useState(false)

    const handleSignout = () => {
        const Store = require("electron-store")
        const storage = new Store()

        new Promise((resolve, _) => {
            storage.set("accessToken", null)
            resolve()
        }).then(() => {
            dispatch(resetState())
            history.push("/")
        })
    }

    return (
        <div>
            <div className={styles.navBar}>
                <div className={styles.logoTitle}>Fractal</div>
                <div
                    style={{
                        position: "absolute",
                        right: "50px",
                        top: "15px",
                        display: "flex",
                        flexDirection: "row",
                        maxWidth: 540,
                    }}
                >
                    {currentTab == "App Store" && (
                        <input
                            value={search}
                            onChange={(evt: any) =>
                                updateSearch(evt.target.value)
                            }
                            placeholder="Search for an app"
                            className={styles.searchBar}
                        />
                    )}
                    <div style={{ display: "flex", flexDirection: "column" }}>
                        <div
                            className={styles.userInfo}
                            onClick={() => setShowProfile(!showProfile)}
                        >
                            <FontAwesomeIcon
                                icon={faUser}
                                style={{
                                    color: "#111111",
                                    fontSize: 30,
                                    padding: 6,
                                    position: "relative",
                                    top: 2,
                                    marginRight: 4,
                                }}
                            />
                            <div
                                style={{
                                    display: "flex",
                                    flexDirection: "column",
                                }}
                            >
                                <span className={styles.name}>{name}</span>
                                <span className={styles.email}>{username}</span>
                            </div>
                        </div>
                        <Collapse in={showProfile}>
                            <div onClick={handleSignout}>
                                <div className={styles.signoutButton}>
                                    Sign Out
                                </div>
                            </div>
                        </Collapse>
                    </div>
                </div>
            </div>
            <div style={{ marginTop: 10, paddingLeft: 45 }}>
                <NavTitle
                    selected={currentTab == "App Store"}
                    text="App Store"
                    onClick={() => updateCurrentTab("App Store")}
                />
                <NavTitle
                    selected={currentTab == "Settings"}
                    text="Settings"
                    onClick={() => updateCurrentTab("Settings")}
                />
                <NavTitle
                    selected={currentTab == "Support"}
                    text="Support"
                    onClick={() => updateCurrentTab("Support")}
                />
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

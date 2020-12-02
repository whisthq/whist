import React, { useState, KeyboardEvent } from "react"
import { connect } from "react-redux"
import { Collapse } from "react-bootstrap"
import { FaUser } from "react-icons/fa"

import { history } from "store/history"
import { resetState } from "store/actions/pure"
import SearchBar from "pages/dashboard/components/searchBar/searchBar"
import { FractalRoute, FractalDashboardTab } from "shared/enums/navigation"

import styles from "pages/dashboard/components/navBar/navBar.css"

const NavTitle = (props: {
    selected: boolean
    text: string
    onClick: (currentTab: string) => void
}) => {
    const { selected, text, onClick, style } = props

    return (
        <div
            className={selected ? styles.selectedNavTitle : styles.navTitle}
            onClick={onClick}
        >
            {text}
        </div>
    )
}

const NavBar = (props: {
    username: string
    name: string
    currentTab: string
    search: string
    updateCurrentTab: (currentTab: string) => void
    updateSearch: (search: string) => void
}) => {
    const {
        username,
        name,
        currentTab,
        search,
        updateCurrentTab,
        updateSearch,
        dispatch,
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
            history.push(FractalRoute.LOGIN)
        })
    }

    return (
        <div>
            <div className={styles.navBar}>
                <div className={styles.logoTitle}>Fractal</div>
                <div className={styles.wrapper}>
                    {currentTab === FractalDashboardTab.APP_STORE && (
                        <div>
                            <SearchBar
                                search={search}
                                callback={(evt: KeyboardEvent) =>
                                    updateSearch(evt.target.value)
                                }
                            />
                        </div>
                    )}
                    <div className={styles.columnFlex}>
                        <div
                            className={styles.userInfo}
                            onClick={() => setShowProfile(!showProfile)}
                        >
                            <FaUser className={styles.faUser} />
                            <div className={styles.columnFlex}>
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
            <div className={styles.tabWrapper}>
                <NavTitle
                    selected={currentTab == FractalDashboardTab.APP_STORE}
                    text={FractalDashboardTab.APP_STORE}
                    onClick={() =>
                        updateCurrentTab(FractalDashboardTab.APP_STORE)
                    }
                />
                <NavTitle
                    selected={currentTab == FractalDashboardTab.SETTINGS}
                    text={FractalDashboardTab.SETTINGS}
                    onClick={() =>
                        updateCurrentTab(FractalDashboardTab.SETTINGS)
                    }
                />
                <NavTitle
                    selected={currentTab == FractalDashboardTab.SUPPORT}
                    text={FractalDashboardTab.SUPPORT}
                    onClick={() =>
                        updateCurrentTab(FractalDashboardTab.SUPPORT)
                    }
                />
            </div>
        </div>
    )
}

const mapStateToProps = <T extends {}>(state: T): T => {
    return {
        username: state.MainReducer.auth.username,
        name: state.MainReducer.auth.name,
    }
}

export default connect(mapStateToProps)(NavBar)

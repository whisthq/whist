import React, { useState, KeyboardEvent, Dispatch } from "react"
import { connect } from "react-redux"
import { Collapse } from "react-bootstrap"
import { FaUser } from "react-icons/fa"

import { history } from "store/history"
import { resetState } from "store/actions/pure"
import SearchBar from "pages/dashboard/components/searchBar/searchBar"
import { FractalRoute, FractalDashboardTab } from "shared/types/navigation"
import { FractalAuthCache } from "shared/types/cache"

import styles from "pages/dashboard/components/navBar/navBar.css"

const NavTitle = (props: { selected: boolean; text: string }) => {
    const { selected, text } = props

    return (
        <div className={selected ? styles.selectedNavTitle : styles.navTitle}>
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
    dispatch: Dispatch
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

        new Promise((resolve) => {
            storage.set(FractalAuthCache.ACCESS_TOKEN, null)
            resolve()
        })
            .then(() => {
                dispatch(resetState())
                history.push(FractalRoute.LOGIN)
                return null
            })
            .catch((err) => {
                throw err
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
                        <button
                            type="button"
                            className={styles.userInfo}
                            onClick={() => setShowProfile(!showProfile)}
                        >
                            <FaUser className={styles.faUser} />
                            <div className={styles.columnFlex}>
                                <span className={styles.name}>{name}</span>
                                <span className={styles.email}>{username}</span>
                            </div>
                        </button>
                        <Collapse in={showProfile}>
                            <button type="button" onClick={handleSignout}>
                                <div className={styles.signoutButton}>
                                    Sign Out
                                </div>
                            </button>
                        </Collapse>
                    </div>
                </div>
            </div>
            <div className={styles.tabWrapper}>
                <button
                    type="button"
                    onClick={() =>
                        updateCurrentTab(FractalDashboardTab.APP_STORE)
                    }
                >
                    <NavTitle
                        selected={currentTab === FractalDashboardTab.APP_STORE}
                        text={FractalDashboardTab.APP_STORE}
                    />
                </button>
                <button
                    type="button"
                    onClick={() =>
                        updateCurrentTab(FractalDashboardTab.SETTINGS)
                    }
                >
                    <NavTitle
                        selected={currentTab === FractalDashboardTab.SETTINGS}
                        text={FractalDashboardTab.SETTINGS}
                    />
                </button>
                <button
                    type="button"
                    onClick={() =>
                        updateCurrentTab(FractalDashboardTab.SUPPORT)
                    }
                >
                    <NavTitle
                        selected={currentTab === FractalDashboardTab.SUPPORT}
                        text={FractalDashboardTab.SUPPORT}
                    />
                </button>
            </div>
        </div>
    )
}

const mapStateToProps = (state: {
    MainReducer: {
        auth: {
            username: string
            name: string
        }
    }
}) => {
    return {
        username: state.MainReducer.auth.username,
        name: state.MainReducer.auth.name,
    }
}

export default connect(mapStateToProps)(NavBar)

import React, { useState } from "react"
import { connect } from "react-redux"
import { Collapse } from "react-bootstrap"
import { FaSearch, FaUser } from "react-icons/fa"

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

const SearchBar = (props: any) => {
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
    const [showSearchBar, setShowSearchBar] = useState(false)

    const toggleSearch = () => {
        setShowSearchBar(!showSearchBar)
    }

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
        <div style={{ maxWidth: 400, display: "flex" }}>
            {currentTab == "App Store" && (
                <input
                    value={search}
                    onChange={(evt: any) => updateSearch(evt.target.value)}
                    placeholder="Search for an app"
                    className={styles.searchBar}
                    style={{
                        width: showSearchBar ? 325 : 0,
                        opacity: showSearchBar ? "1.0" : "0.0",
                        padding: showSearchBar ? "5px 10px" : "5px 0px",
                    }}
                />
            )}
            <FaSearch
                style={{
                    position: "relative",
                    top: 3,
                    marginRight: 12,
                    cursor: "pointer",
                    fontSize: 29,
                    padding: 6,
                    color: showSearchBar ? "#111111" : "#555555",
                    zIndex: 2,
                    right: showSearchBar ? 5 : 0,
                    transition: "1.5s",
                }}
                onClick={toggleSearch}
            />
        </div>
    )
}

export default connect(mapStateToProps)(SearchBar)

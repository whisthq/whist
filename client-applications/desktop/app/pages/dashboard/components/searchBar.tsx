import React, { useState } from "react"
import { FaSearch } from "react-icons/fa"

import styles from "styles/dashboard.css"

const SearchBar = (props: { search: string; callback: (evt: any) => void }) => {
    const { search, callback } = props
    const [showSearchBar, setShowSearchBar] = useState(false)

    const toggleSearch = () => {
        setShowSearchBar(!showSearchBar)
    }

    return (
        <div style={{ maxWidth: 400, display: "flex" }}>
            <input
                value={search}
                onChange={callback}
                placeholder="Search for an app"
                className={styles.searchBar}
                style={{
                    width: showSearchBar ? 325 : 0,
                    opacity: showSearchBar ? "1.0" : "0.0",
                    padding: showSearchBar ? "5px 10px" : "5px 0px",
                }}
            />
            <FaSearch
                style={{
                    position: "relative",
                    marginRight: 12,
                    cursor: "pointer",
                    zIndex: 2,
                    padding: 6,
                    transition: "1s",
                    top: showSearchBar ? 4 : 3,
                    fontSize: showSearchBar ? 26 : 29,
                    color: showSearchBar ? "#111111" : "#555555",
                    right: showSearchBar ? 5 : 0,
                }}
                onClick={toggleSearch}
            />
        </div>
    )
}

export default SearchBar

import React, { useState } from "react"
import { FaSearch } from "react-icons/fa"

import styles from "pages/dashboard/components/searchBar/searchBar.css"

const SearchBar = (props: {
    search: string
    callback: (evt: KeyboardEvent) => void
}) => {
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
                className={
                    showSearchBar ? styles.searchBar : styles.searchBarHidden
                }
            />
            <FaSearch
                className={
                    showSearchBar ? styles.faSearchActive : styles.faSearch
                }
                onClick={toggleSearch}
            />
        </div>
    )
}

export default SearchBar

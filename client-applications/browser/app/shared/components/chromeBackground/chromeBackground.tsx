import React from "react"
import {
    FaArrowLeft,
    FaArrowRight,
    FaRedo,
    FaEllipsisV,
    FaPlus,
} from "react-icons/fa"

import styles from "shared/components/chromeBackground/chromeBackground.css"

export const ChromeBackground = () => {
    return (
        <>
            <div className={styles.titlebar}></div>
            <div style={{ position: "relative", top: 38 }}>
                <div className={styles.innerRound}></div>
                <div className={styles.tab}></div>
                <div
                    className={styles.innerRound}
                    style={{ left: 259, transform: "scaleX(-1)" }}
                ></div>
                <FaPlus className={styles.addTab} />
                <div className={styles.searchBarWrapper}>
                    <div>
                        <FaArrowLeft className={styles.navigationIcon} />
                    </div>
                    <div>
                        <FaArrowRight
                            className={styles.navigationIcon}
                            style={{ color: "#eeeeee" }}
                        />
                    </div>
                    <div>
                        <FaRedo
                            style={{
                                fontSize: 12,
                                top: 6,
                            }}
                            className={styles.navigationIcon}
                        />
                    </div>
                    <div className={styles.searchBar}></div>
                    <div>
                        <FaEllipsisV
                            className={styles.navigationIcon}
                            style={{
                                fontSize: 13,
                                top: 7,
                                color: "#666666",
                                marginRight: 0,
                                paddingLeft: 20,
                            }}
                        />
                    </div>
                </div>
            </div>
        </>
    )
}

export default ChromeBackground

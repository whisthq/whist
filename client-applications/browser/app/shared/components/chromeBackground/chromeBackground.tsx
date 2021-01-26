import React from "react"
import {
    FaArrowLeft,
    FaArrowRight,
    FaRedo,
    FaEllipsisV,
    FaPlus,
    FaTimes,
} from "react-icons/fa"

import { FractalIPC } from "shared/types/ipc"

import styles from "shared/components/chromeBackground/chromeBackground.css"

export const ChromeBackground = () => {
    /*
        Background that looks like an empty Chrome tab
 
        Arguments: none
    */

    // const ipc = require("electron").ipcRenderer

    // const closeTab = () => {
    //     ipc.sendSync(FractalIPC.FORCE_QUIT)
    // }

    return (
        <>
            <div className={styles.titlebar} />
            <div style={{ position: "relative", top: 38 }}>
                <div className={styles.innerRound} />
                <div className={styles.tab} />
                <div className={styles.tabCloseWrapper}>
                    <FaTimes className={styles.tabClose} />
                </div>
                <div
                    className={styles.innerRound}
                    style={{ left: 259, transform: "scaleX(-1)" }}
                />
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
                    <div className={styles.searchBar} />
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

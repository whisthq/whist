import React from "react"
import { connect } from "react-redux"
import {
    FaArrowLeft,
    FaArrowRight,
    FaRedo,
    FaEllipsisV,
    FaPlus,
    FaTimes,
} from "react-icons/fa"

import { ComputerInfo } from "store/reducers/client/default"
import { OperatingSystem } from "shared/types/client"
import { FractalIPC } from "shared/types/ipc"

import styles from "shared/components/chromeBackground/chromeBackground.css"

export const ChromeBackground = (props: {
    operatingSystem: OperatingSystem | undefined
}) => {
    /*
        Background that looks like an empty Chrome tab
 
        Arguments: none
    */

    const { operatingSystem } = props
    const ipc = require("electron").ipcRenderer

    const closeTab = () => {
        ipc.sendSync(FractalIPC.FORCE_QUIT)
    }

    return (
        <>
            <div className={styles.titlebar} />
            <div style={{ position: "absolute", top: 38, left: 0, width: "100vw" }}>
                <div
                    style={{
                        left:
                            operatingSystem === OperatingSystem.WINDOWS
                                ? 0
                                : 80,
                        position: "absolute",
                    }}
                >
                    <div className={styles.innerRound} />
                    <div className={styles.tab} />
                    <div className={styles.tabCloseWrapper} onClick={closeTab}>
                        <FaTimes className={styles.tabClose} />
                    </div>
                    <div
                        className={styles.innerRound}
                        style={{ left: 259, transform: "scaleX(-1)" }}
                    />
                    <FaPlus className={styles.addTab} />
                </div>
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

const mapStateToProps = (state: {
    ClientReducer: { computerInfo: ComputerInfo }
}) => {
    return {
        operatingSystem: state.ClientReducer.computerInfo.operatingSystem,
    }
}

export default connect(mapStateToProps)(ChromeBackground)

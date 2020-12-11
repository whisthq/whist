import React from "react"
import { connect } from "react-redux"
import styles from "pages/login/login.css"
import Titlebar from "react-electron-titlebar"

import { OperatingSystem } from "shared/types/client"

const TitleBar = (props: { clientOS: string }) => {
    const { clientOS } = props

    return (
        <>
            {clientOS === OperatingSystem.WINDOWS ? (
                <div>
                    <Titlebar backgroundColor="#000000" />
                </div>
            ) : (
                <div className={styles.macTitleBar} />
            )}
        </>
    )
}

export const mapStateToProps = <T extends {}>(state: T) => {
    return {
        clientOS: state.MainReducer.client.clientOS,
    }
}

export default connect(mapStateToProps)(TitleBar)

import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { FaPlus } from "react-icons/fa"
import { FaMinus } from "react-icons/fa"

import styles from "pages/onboard/onboard.css"

const Apps = (props: { app: any; selected: boolean }) => {
    const { app, selected } = props

    return (
        <div
            className={styles.appContainer}
            style={{ background: selected ? "#3930b8" : "" }}
        >
            <img src={app.logo_url} className={styles.appImage} />
            <div className={styles.plusButton}>
                {selected ? (
                    <FaMinus className={styles.faButton} />
                ) : (
                    <FaPlus className={styles.faButton} />
                )}
            </div>
        </div>
    )
}

export const mapStateToProps = <T extends {}>(state: T) => {
    return {
        username: state.MainReducer.auth.username,
        name: state.MainReducer.auth.name,
        os: state.MainReducer.client.os,
        accessToken: state.MainReducer.auth.accessToken,
    }
}

export default connect(mapStateToProps)(Apps)

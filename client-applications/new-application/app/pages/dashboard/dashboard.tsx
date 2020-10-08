import React from 'react'
import { connect } from 'react-redux'
import styles from 'styles/dashboard.css'
import Titlebar from 'react-electron-titlebar'

import NavBar from './components/navBar'
import Discover from './pages/discover'

const Dashboard = (props: any) => {
    const { dispatch, username, os } = props

    return (
        <>
            {os === 'win32' ? (
                <div>
                    <Titlebar backgroundColor="#000000" />
                </div>
            ) : (
                <div className={styles.macTitleBar} />
            )}
            <div
                style={{ display: 'flex', flexDirection: 'row' }}
                className={styles.removeDrag}
            >
                <NavBar />
                <Discover />
            </div>
        </>
    )
}

const mapStateToProps = (state: any) => {
    return {
        username: state.MainReducer.auth.username,
        os: state.MainReducer.client.os,
    }
}

export default connect(mapStateToProps)(Dashboard)

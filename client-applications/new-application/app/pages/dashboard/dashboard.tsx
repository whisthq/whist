import React from 'react'
import { connect } from 'react-redux'
import { history } from 'store/configureStore'
import styles from 'styles/dashboard.css'

import { logout } from 'store/actions/main'

const Dashboard = (props: any) => {
    const { dispatch, username } = props

    const handleSignout = () => {
        const storage = require('electron-json-storage')
        storage.set('credentials', { username: '', password: '' })
        dispatch(logout())
        history.push('/')
    }

    return (
        <div
            style={{
                marginTop: 200,
                textAlign: 'center',
                display: 'flex',
                flexDirection: 'column',
                alignItems: 'center',
            }}
        >
            New dashboard here!
            <div>Username: {username}</div>
            <button
                type="button"
                onClick={handleSignout}
                className={styles.signoutButton}
                id="signout-button"
                style={{
                    textAlign: 'center',
                }}
            >
                SIGN OUT
            </button>
        </div>
    )
}

const mapStateToProps = (state: any) => {
    return { username: state.MainReducer.username }
}

export default connect(mapStateToProps)(Dashboard)

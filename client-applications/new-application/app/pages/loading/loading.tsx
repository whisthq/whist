import React, { useState, useEffect } from 'react'
import { connect } from 'react-redux'
import styles from 'styles/login.css'
import Titlebar from 'react-electron-titlebar'
import Logo from 'assets/images/logo.svg'
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faCircleNotch } from '@fortawesome/free-solid-svg-icons'

import { fetchContainer } from 'store/actions/counter'

const UpdateScreen = (props: any) => {
    const { os, dispatch, percentLoaded, status } = props

    const [percentLoadedWidth, setPercentLoadedWidth] = useState(0)
    const [percentLeftWidth, setPercentLeftWidth] = useState(300)

    useEffect(() => {
        dispatch(fetchContainer())
    }, [])

    useEffect(() => {
        if (percentLoadedWidth < percentLoaded * 3) {
            const newWidth = percentLoadedWidth + 2
            setPercentLoadedWidth(newWidth)
            setPercentLeftWidth(300 - newWidth)
        }
    }, [percentLoaded, percentLoadedWidth])

    return (
        <div
            style={{
                position: 'fixed',
                top: 0,
                left: 0,
                width: 1000,
                height: 680,
                backgroundColor: '#0B172B',
                zIndex: 1000,
            }}
        >
            {os === 'win32' ? (
                <div>
                    <Titlebar backgroundColor="#000000" />
                </div>
            ) : (
                <div style={{ marginTop: 10 }}></div>
            )}
            <div className={styles.landingHeader}>
                <div className={styles.landingHeaderLeft}>
                    <img src={Logo} width="20" height="20" />
                    <span className={styles.logoTitle}>Fractal</span>
                </div>
            </div>
            <div style={{ position: 'relative' }}>
                <div
                    style={{
                        paddingTop: 180,
                        textAlign: 'center',
                        color: 'white',
                        width: 1000,
                    }}
                >
                    <div
                        style={{
                            display: 'flex',
                            alignItems: 'center',
                            justifyContent: 'center',
                        }}
                    >
                        <div
                            style={{
                                width: `${percentLoadedWidth}px`,
                                height: 6,
                                background:
                                    'linear-gradient(258.54deg, #5ec3eb 0%, #d023eb 100%)',
                            }}
                        ></div>
                        <div
                            style={{
                                width: `${percentLeftWidth}px`,
                                height: 6,
                                background: '#111111',
                            }}
                        ></div>
                    </div>
                    <div
                        style={{
                            marginTop: 10,
                            fontSize: 14,
                            display: 'flex',
                            alignItems: 'center',
                            justifyContent: 'center',
                        }}
                    >
                        <div style={{ color: '#D6D6D6' }}>
                            {status != 'SUCCESS.' && (
                                <FontAwesomeIcon
                                    icon={faCircleNotch}
                                    spin
                                    style={{
                                        color: '#5EC4EB',
                                        marginRight: 4,
                                        width: 12,
                                    }}
                                />
                            )}{' '}
                            {status}
                        </div>
                    </div>
                </div>
            </div>
        </div>
    )
}

function mapStateToProps(state: any) {
    return {
        os: state.counter.os,
        percentLoaded: state.counter.percent_loaded,
        status: state.counter.status_message,
    }
}

export default connect(mapStateToProps)(UpdateScreen)

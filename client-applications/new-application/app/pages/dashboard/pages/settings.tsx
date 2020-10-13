import React, { useState } from 'react'
import { Row } from 'react-bootstrap'
import { connect } from 'react-redux'
import ToggleButton from 'react-toggle-button'
import Slider from 'react-input-slider'
import styles from 'styles/dashboard.css'

import Wifi from 'assets/images/wifi.svg'
import Speedometer from 'assets/images/speedometer.svg'

const Settings = (props: any) => {
    const [lowInternetMode, setLowInternetMode] = useState(false)
    const [bandwidth, setBandwidth] = useState(500)

    const toggleLowInternetMode = (mode: boolean) => {
        const storage = require('electron-json-storage')
        setLowInternetMode(!mode)
        storage.set('settings', { lowInternetMode: !mode })
    }

    const changeBandwidth = (mbps: number) => {
        const storage = require('electron-json-storage')
        setBandwidth(mbps)
        mbps = mbps === 50 ? 500 : mbps
        storage.set('settings', { bandwidth: mbps })
    }

    return (
        <div className={styles.page}>
            <h2>Settings</h2>
            <Row
                style={{
                    padding: '20px',
                    display: 'flex',
                    flexDirection: 'row',
                    marginTop: '75px',
                }}
            >
                <div style={{ width: '75%' }}>
                    <div
                        style={{
                            color: '#111111',
                            fontSize: 16,
                            fontWeight: 'bold',
                        }}
                    >
                        <img
                            src={Wifi}
                            style={{
                                color: '#111111',
                                height: 14,
                                marginRight: 12,
                                position: 'relative',
                                top: 2,
                                width: 16,
                            }}
                        />
                        Low Internet Mode
                    </div>
                    <div
                        style={{
                            fontSize: 13,
                            color: '#333333',
                            marginTop: 10,
                            marginLeft: 28,
                            lineHeight: 1.4,
                        }}
                    >
                        If you have low Internet speeds (less than 15 Mbps) and
                        are experiencing stuttering, low Internet mode will
                        reduce Fractal's Internet speed requirement. Note that
                        this will increase your latency by a very small amount,
                        so we don't recommend activating it unless necessary.
                    </div>
                </div>
                <div style={{ width: '25%' }}>
                    <div style={{ float: 'right', marginTop: '25px' }}>
                        <ToggleButton
                            value={lowInternetMode}
                            onToggle={toggleLowInternetMode}
                            colors={{
                                active: {
                                    base: '#93f1d9',
                                },
                                inactive: {
                                    base: '#161936',
                                },
                            }}
                            style={{ height: '40px' }}
                        />
                    </div>
                </div>
            </Row>
            <Row
                style={{
                    padding: '20px',
                    display: 'flex',
                    flexDirection: 'row',
                }}
            >
                <div style={{ width: '75%' }}>
                    <div
                        style={{
                            color: '#111111',
                            fontSize: 16,
                            fontWeight: 'bold',
                        }}
                    >
                        <img
                            src={Speedometer}
                            style={{
                                color: '#111111',
                                height: 14,
                                marginRight: 12,
                                position: 'relative',
                                top: 2,
                                width: 16,
                            }}
                        />
                        Maximum Bandwidth
                    </div>
                    <div
                        style={{
                            fontSize: 13,
                            color: '#333333',
                            marginTop: 10,
                            marginLeft: 28,
                            lineHeight: 1.4,
                        }}
                    >
                        Toggle the maximum bandwidth (Mbps) that Fractal
                        consumes. We recommend adjusting this setting only if
                        you are also running other, bandwidth-consuming apps.
                    </div>
                </div>
                <div
                    style={{
                        width: '25%',
                        display: 'flex',
                        flexDirection: 'column',
                        justifyContent: 'space-around',
                        alignItems: 'flex-end',
                        padding: '10px 0px',
                    }}
                >
                    <div style={{ float: 'right' }}>
                        <Slider
                            axis="x"
                            xstep={5}
                            xmin={10}
                            xmax={50}
                            x={bandwidth}
                            onChange={({ x }) => changeBandwidth(x)}
                            styles={{
                                track: {
                                    backgroundColor: '#cccccc',
                                    height: 5,
                                    width: 100,
                                },
                                active: {
                                    backgroundColor: '#93f1d9',
                                    height: 5,
                                },
                                thumb: {
                                    height: 10,
                                    width: 10,
                                },
                            }}
                        />
                    </div>
                    <div
                        style={{
                            fontSize: 11,
                            color: '#333333',
                            float: 'right',
                            textAlign: 'right',
                        }}
                    >
                        {bandwidth < 50 ? (
                            <div>{bandwidth} mbps</div>
                        ) : (
                            <div>Unlimited</div>
                        )}
                    </div>
                </div>
            </Row>
        </div>
    )
}

const mapStateToProps = (state: any) => {
    return {}
}

export default connect(mapStateToProps)(Settings)

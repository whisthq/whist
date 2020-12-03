import React, { useState, useEffect } from "react"
import { Row, Form, Button, Alert, Dropdown } from "react-bootstrap"
import { connect } from "react-redux"
import ToggleButton from "react-toggle-button"
import Slider from "react-input-slider"
import { FaWifi, FaNetworkWired } from "react-icons/fa"

import { FractalClientCache } from "shared/enums/cache"

import styles from "pages/dashboard/views/settings/settings.css"
import dashboardStyles from "pages/dashboard/dashboard.css"

import { updateAdmin } from "store/actions/pure"
import Fractal from "assets/images/fractal.svg"

import { DEFAULT } from "store/reducers/states"

const AdminDropdown = (props: {
    // how to modify the state
    onClick: (value: any) => any
    // it will display these plus a text box plus a reset button
    // the reset button will set the value to the defaultValue
    options: string[]
    defaultValue: string | null
    // value is what will be the dropdown value to be selected at this moment
    value: string | null
    // title will basically say what type it is i.e. webserver or cluster or etc...
    title: string
}) => {
    const { onClick, options, defaultValue, value, title } = props
    const reset = "Reset"
    const displayOptions = [...options]
    displayOptions.push(reset)

    const [customValue, setCustomValue] = useState("")
    const validCustomValue = customValue.trim() !== "" && customValue.length > 3

    const changeCustomValue = (evt: any) => {
        console.log(`hey ${evt.target.value}`)
        setCustomValue(evt.target.value)
    }

    const handleDropdownClick = (option: string) => {
        //console.log(`option ${option} was clicked!`)
        if (option == reset) {
            console.log(
                `resetting ${title} to default value of ${defaultValue}`
            )
            onClick(defaultValue)
        } else {
            console.log(`setting to ${option}`)
            onClick(option)
        }
    }

    const handleCustomClick = () => {
        if (validCustomValue) {
            onClick(customValue)
        }
    }

    return (
        <div style={{ marginTop: 20, width: "200%" }}>
            <Dropdown>
                <Dropdown.Toggle variant="primary" id={title}>
                    {title}
                </Dropdown.Toggle>

                <Dropdown.Menu>
                    {displayOptions.map((option: string) => (
                        <Dropdown.Item
                            key={option}
                            active={option == value}
                            onClick={() => handleDropdownClick(option)}
                        >
                            {option}
                        </Dropdown.Item>
                    ))}
                </Dropdown.Menu>
            </Dropdown>
            <Form>
                <Form.Group controlId={title + "CustomValue"}>
                    <Form.Label>Custom Value</Form.Label>
                    <Form.Control
                        placeholder="Enter Custom Value"
                        value={customValue}
                        onChange={changeCustomValue}
                    />
                    <Form.Text className="text-muted">
                        {title}: At least three non-whitespace characters.
                    </Form.Text>
                </Form.Group>
                <Button
                    variant="primary"
                    onClick={handleCustomClick}
                    disabled={!validCustomValue}
                >
                    Set Custom Value
                </Button>
            </Form>
        </div>
    )
}

const AdminSettings = (props: any) => {
    const { dispatch, adminState } = props

    const [region, setRegion] = useState(adminState.region)
    const [task, setTask] = useState(adminState.task_arn)
    const [webserver, setWebserver] = useState(adminState.webserver_url)
    const [cluster, setCluster] = useState(adminState.cluster)

    //here are some preset options for the dropdown
    const regions = ["us-east-1", "us-west-1", "ca-central-1"]

    const webservers = ["local", "dev", "staging", "prod"]

    // no cluster defaults

    const tasks = [
        // browsers
        "fractal-browsers-brave",
        "fractal-browsers-chrome",
        "fractal-browsers-firefox",
        "fractal-browsers-sidekick",
        //creative
        "fractal-creative-blender",
        "fractal-creative-blockbench",
        "fractal-creative-figma",
        "fractal-creative-gimp",
        "fractal-creative-lightworks",
        "fractal-creative-texturelab",
        //productivity
        "fractal-productivity-discord",
        "fractal-productivity-notion",
        "fractal-productivity-slack",
    ]

    const handleSaveTask = (value: any) => {
        setTask(value)
        dispatch(
            updateAdmin({
                task_arn: value,
            })
        )
    }

    const handleSaveRegion = (value: any) => {
        setRegion(value)
        dispatch(
            updateAdmin({
                region: value,
            })
        )
    }

    const handleSaveWebserver = (value: any) => {
        setWebserver(value)
        dispatch(
            updateAdmin({
                webserver_url: value,
            })
        )
    }

    const handleSaveCluster = (value: any) => {
        setCluster(value)
        dispatch(
            updateAdmin({
                cluster: value,
            })
        )
    }
    return (
        <div>
            <Row
                style={{
                    display: "flex",
                    flexDirection: "column",
                    marginTop: 50,
                    marginBottom: 25,
                }}
            >
                <div style={{ width: "75%" }}>
                    <div
                        style={{
                            color: "#111111",
                            fontSize: 16,
                            fontWeight: "bold",
                        }}
                    >
                        <img
                            src={Fractal}
                            style={{
                                color: "#111111",
                                height: 14,
                                marginRight: 12,
                                position: "relative",
                                top: 2,
                                width: 16,
                            }}
                        />
                        (Admin Only) Test Settings
                    </div>
                    <div
                        style={{
                            fontSize: 13,
                            color: "#333333",
                            marginTop: 10,
                            marginLeft: 28,
                            lineHeight: 1.4,
                        }}
                    >
                        Give a region (like us-east-1) and a webserver version
                        (local, dev, staging, prod, or a url) and save them to
                        choose where to connect to. In future may support
                        different task defs. If the region is not a valid region
                        the change does not go through (silently).
                    </div>
                </div>
                <div
                    style={{
                        width: "25%",
                        display: "flex",
                        flexDirection: "column",
                        justifyContent: "space-around",
                        alignItems: "flex-end",
                        padding: "10px 0px",
                    }}
                >
                    <div style={{ float: "right" }}>
                        <AdminDropdown
                            onClick={handleSaveWebserver}
                            options={webservers}
                            title="Webserver"
                            defaultValue={DEFAULT.admin.webserver_url}
                            value={webserver}
                        />
                        <AdminDropdown
                            onClick={handleSaveRegion}
                            options={regions}
                            title="Region"
                            defaultValue={DEFAULT.admin.region}
                            value={region}
                        />
                        <AdminDropdown
                            onClick={handleSaveTask}
                            options={tasks}
                            title="Task"
                            defaultValue={DEFAULT.admin.task_arn}
                            value={task}
                        />
                        <AdminDropdown
                            onClick={handleSaveCluster}
                            options={[]}
                            title="Cluster"
                            defaultValue={DEFAULT.admin.cluster}
                            value={cluster}
                        />
                    </div>
                </div>
            </Row>
        </div>
    )
}

const Settings = <T extends {}>(props: T) => {
    const [lowInternetMode, setLowInternetMode] = useState(false)
    const [bandwidth, setBandwidth] = useState(500)
    const [showSavedAlert, setShowSavedAlert] = useState(false)

    const adminUsername =
        props.username &&
        props.username.indexOf("@") > -1 &&
        props.username.split("@")[1] == "tryfractal.com"

    useEffect(() => {
        const Store = require("electron-store")
        const storage = new Store()

        const lowInternetMode = storage.get(
            FractalClientCache.LOW_INTERNET_MODE
        )
        const bandwidth = storage.get(FractalClientCache.BANDWIDTH)

        if (lowInternetMode) {
            setLowInternetMode(lowInternetMode)
        }
        if (bandwidth) {
            setBandwidth(bandwidth)
        }
    }, [])

    const toggleLowInternetMode = (mode: boolean) => {
        setLowInternetMode(!mode)
    }

    const changeBandwidth = (mbps: number) => {
        setBandwidth(mbps)
    }

    const handleSave = () => {
        const Store = require("electron-store")
        const storage = new Store()
        const mbps = bandwidth === 50 ? 500 : bandwidth

        storage.set(FractalClientCache.LOW_INTERNET_MODE, lowInternetMode)
        storage.set(FractalClientCache.BANDWIDTH, mbps)

        setShowSavedAlert(true)
    }

    return (
        <div
            className={dashboardStyles.page}
            style={{
                // might be nicer to add this to page but not sure if you prefer not to
                overflowY: "scroll",
                maxHeight: 525,
            }}
        >
            <Row className={styles.row}>
                <div style={{ width: "75%" }}>
                    <div className={styles.header}>
                        <FaWifi className={styles.faIcon} />
                        Low Internet Mode
                    </div>
                    <div className={styles.text}>
                        If you have low Internet speeds (less than 15 Mbps), low
                        Internet mode will reduce Fractal's Internet speed
                        requirement. This will increase your latency slightly,
                        so we don't recommend activating it unless necessary.
                    </div>
                </div>
                <div style={{ width: "25%" }}>
                    <div style={{ float: "right", marginTop: "25px" }}>
                        <ToggleButton
                            value={lowInternetMode}
                            onToggle={toggleLowInternetMode}
                            colors={{
                                active: {
                                    base: "#93f1d9",
                                },
                                inactive: {
                                    base: "#161936",
                                },
                            }}
                            style={{ height: "40px" }}
                        />
                    </div>
                </div>
            </Row>
            <Row
                className={styles.row}
                style={{
                    marginBottom: 25,
                }}
            >
                <div style={{ width: "75%" }}>
                    <div className={styles.header}>
                        <FaNetworkWired className={styles.faIcon} />
                        Maximum Bandwidth
                    </div>
                    <div className={styles.text}>
                        Toggle the maximum bandwidth (Mbps) that Fractal
                        consumes. We recommend adjusting this setting only if
                        you are also running other, bandwidth-consuming apps.
                    </div>
                </div>
                <div className={styles.sliderWrapper}>
                    <div style={{ float: "right" }}>
                        <Slider
                            axis="x"
                            xstep={5}
                            xmin={10}
                            xmax={50}
                            x={bandwidth}
                            onChange={({ x }) => changeBandwidth(x)}
                            styles={{
                                track: {
                                    backgroundColor: "#cccccc",
                                    height: 5,
                                    width: 100,
                                },
                                active: {
                                    backgroundColor: "#93f1d9",
                                    height: 5,
                                },
                                thumb: {
                                    height: 10,
                                    width: 10,
                                },
                            }}
                        />
                    </div>
                    <div className={styles.sliderPopup}>
                        {bandwidth < 50 ? (
                            <div>{bandwidth} mbps</div>
                        ) : (
                            <div>Unlimited</div>
                        )}
                    </div>
                </div>
            </Row>
            {showSavedAlert && (
                <Row>
                    <Alert
                        variant="success"
                        onClose={() => setShowSavedAlert(false)}
                        dismissible
                        className={styles.alert}
                    >
                        Your settings have been saved!
                    </Alert>
                </Row>
            )}
            <Row style={{ justifyContent: "flex-end" }}>
                <div
                    className={dashboardStyles.feedbackButton}
                    style={{ width: 110, marginTop: 25 }}
                    onClick={handleSave}
                >
                    Save
                </div>
            </Row>
            {adminUsername && (
                <AdminSettings
                    dispatch={props.dispatch}
                    adminState={props.adminState}
                />
            )}
        </div>
    )
}

const mapStateToProps = <T extends {}>(state: T): T => {
    return {
        username: state.MainReducer.auth.username,
        adminState: state.MainReducer.admin,
    }
}

export default connect(mapStateToProps)(Settings)

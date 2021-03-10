import React, { useState, useEffect, ChangeEvent, Dispatch } from "react"
import { connect } from "react-redux"
import { useMutation } from "@apollo/client"
import { FaPencilAlt } from "react-icons/fa"

import Input from "shared/components/input"
import * as PureAuthAction from "store/actions/auth/pure"
import { UPDATE_NAME } from "shared/constants/graphql"
import { User } from "shared/types/reducers"

import { E2E_DASHBOARD_IDS, PROFILE_IDS } from "testing/utils/testIDs"

import styles from "styles/profile.module.css"
import sharedStyles from "styles/shared.module.css"
import classNames from "classnames"

const NameForm = (props: { dispatch: Dispatch<any>; user: User }) => {
    const { dispatch, user } = props

    const [completed, setCompleted] = useState(false)
    const [error, setError] = useState(false)

    const [updateName] = useMutation(UPDATE_NAME, {
        context: {
            headers: {
                Authorization: `Bearer ${user.accessToken}`,
            },
        },
        onCompleted: () => {
            setCompleted(true)
        },
        onError: () => {
            setError(true)
        },
    })

    const [editingName, setEditingName] = useState(false)
    const [newName, setNewName] = useState(user.name)
    const [savedName, setSavedName] = useState(false)

    const changeNewName = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setNewName(evt.target.value)
    }

    useEffect(() => {
        let name: string | undefined
        if (newName !== null) {
            name = newName as string
        }
        if (completed && !error) {
            setEditingName(false)
            dispatch(PureAuthAction.updateUser({ name: name }))
            setSavedName(true)
        }
    }, [completed, error, dispatch, newName])

    // Update user's name in database and redux state
    const saveNewName = () => {
        setCompleted(false)
        setError(false)
        updateName({
            variables: {
                userID: user.userID,
                name: newName,
            },
        })
    }

    const handleCancel = () => {
        setCompleted(false)
        setError(false)
        setEditingName(false)
    }

    return (
        <div style={{ marginTop: 15 }} data-testid={PROFILE_IDS.NAME}>
            <div className={classNames("font-body", styles.sectionTitle)}>Name</div>
            <div className={styles.sectionInfo}>
                {editingName ? (
                    <div
                        className={styles.dashedBox}
                        style={{
                            display: "flex",
                            flexDirection: "column",
                            width: "100%",
                            padding: "15px 20px",
                        }}
                    >
                        <Input
                            id={E2E_DASHBOARD_IDS.NAMEFIELD}
                            type="name"
                            placeholder={"New name"}
                            onChange={changeNewName}
                            value={newName ? newName : ""}
                            warning={error ? "Unable to save name." : ""}
                        />
                        <div
                            style={{
                                display: "flex",
                                flexDirection: "row",
                                justifyContent: "space-between",
                            }}
                        >
                            <button
                                className={classNames("font-body", styles.saveButton)}
                                data-testid={PROFILE_IDS.SAVE}
                                onClick={saveNewName}
                            >
                                SAVE
                            </button>
                            <button
                                className={classNames("font-body", sharedStyles.whiteButton)}
                                style={{
                                    width: "47%",
                                    padding: "10px 0px",
                                    fontSize: "16px",
                                    marginTop: "20px",
                                }}
                                onClick={handleCancel}
                            >
                                CANCEL
                            </button>
                        </div>
                    </div>
                ) : (
                    <>
                        {user.name ? (
                            <div
                                className={styles.dashedBox}
                                style={{
                                    position: "relative",
                                    display: "flex",
                                    justifyContent: "space-between",
                                }}
                            >
                                <div className="font-body">{user.name} </div>
                                <div>
                                    {savedName && (
                                        <div className={styles.saved}>
                                            Saved!
                                        </div>
                                    )}
                                    <FaPencilAlt
                                        id={E2E_DASHBOARD_IDS.EDITNAME}
                                        className="relative top-1 cursor-pointer"
                                        onClick={() => {
                                            setEditingName(true)
                                            setSavedName(false)
                                        }}
                                    />
                                </div>
                            </div>
                        ) : (
                            <div
                                className={classNames("font-body", styles.dashedBox)}
                                onClick={() => {
                                    setEditingName(true)
                                    setSavedName(false)
                                }}
                                style={{ cursor: "pointer" }}
                            >
                                + Add your name
                            </div>
                        )}
                    </>
                )}
            </div>
        </div>
    )
}

const mapStateToProps = (state: { AuthReducer: { user: User } }) => {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(NameForm)

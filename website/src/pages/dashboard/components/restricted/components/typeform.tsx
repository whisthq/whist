import React from "react"
import { connect } from "react-redux"
import Modal from "react-modal"
import {FaTimes} from "react-icons/fa"
import { useMutation } from "@apollo/client"
import { ReactTypeformEmbed } from "react-typeform-embed"

import "reactjs-popup/dist/index.css"

import { debugLog } from "shared/utils/logging"
import { User } from "store/reducers/auth/default"
import { INSERT_INVITE, UPDATE_INVITE } from "shared/constants/graphql"
import { config } from "shared/constants/config"

const Typeform = (props: {
    user: User
    show: boolean
    handleClose: () => void
}) => {
    /*
        React component for embedded Typeform modal

        Arguments:
            user (User): Current logged in user
            show (boolean): True/false to show the modal
            handleClose (() => void): Callback function on modal close
    */

    const { user, show, handleClose } = props

    const [insertInvite] = useMutation(INSERT_INVITE, {
        context: {
            headers: {
                Authorization: `Bearer ${user.accessToken}`,
            },
        },
    })

    const [updateInvite] = useMutation(UPDATE_INVITE, {
        context: {
            headers: {
                Authorization: `Bearer ${user.accessToken}`
            }
        }
    })

    const onSubmit = () => {
        insertInvite({
            variables: {
                userID: user.userID,
            },
        }).catch(() => {
            updateInvite({
                variables: {
                    userID: user.userID,
                    typeformSubmitted: true
                }
            }).catch((err) => {
                debugLog(err)
            })
        })
    }

    return (
        <div className="text-center">
            <Modal isOpen={show} onRequestClose={handleClose} style ={{content: {
                maxWidth: 900,
                maxHeight: 600,
                margin: "auto"
            }}}>
                <div>
                    <ReactTypeformEmbed
                        url={config.url.TYPEFORM_URL}
                        popup={false}
                        onSubmit={onSubmit}
                        style={{
                            maxWidth: 900,
                            maxHeight: 600
                        }}
                    />
                    <div className="absolute right-4 top-2 w-8 h-8 z-50 cursor-pointer" onClick={handleClose}>
                        <FaTimes className="relative top-2 left-2 hover:text-blue duration-500 text-2xl"/>
                    </div>
                </div>
            </Modal>
        </div>
    )
}

const mapStateToProps = (state: { AuthReducer: { user: User } }) => {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Typeform)

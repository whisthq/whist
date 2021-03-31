import React, { useState } from "react"
import Modal from "react-modal"
import { FaTimes } from "react-icons/fa"
import TypeformComponent from "react-typeform-embed"
import classNames from "classNames"

import "reactjs-popup/dist/index.css"

import { config } from "@app/shared/constants/config"

const Typeform = (props: {
    show: boolean
    onClose: () => void
    onSubmit?: () => void
}) => {
    /*
        React component for embedded Typeform modal
        Arguments:
            user (User): Current logged in user
            show (boolean): True/false to show the modal
            handleClose (() => void): Callback function on modal close
    */

    const { show, onClose, onSubmit } = props

    return (
        <div className="text-center">
            <Modal
                isOpen={show}
                onRequestClose={onClose}
                style={{
                    content: {
                        maxWidth: "100vw",
                        maxHeight: "100vh",
                        margin: "auto"
                    },
                }}
            >
                <div>
                    <TypeformComponent.ReactTypeformEmbed
                        url={config.url.TYPEFORM_URL}
                        popup={false}
                        onSubmit={onSubmit}
                    />
                    <div
                        className="absolute right-4 top-2 w-8 h-8 z-50 cursor-pointer"
                        onClick={onClose}
                    >
                        <FaTimes className="relative top-2 left-2 hover:text-blue duration-500 text-2xl" />
                    </div>
                </div>
            </Modal>
        </div>
    )
}

const TypeformButton = (props: {
    text: string
}) => {
    const {text} = props
    const [show, setShow] = useState(false)

    return (
        <>
            <Typeform show={show} onClose={() => setShow(false)} />
            <button
                className={classNames(
                    "relative text-gray-100 rounded bg-blue dark:bg-mint dark:text-black px-8 py-3 mt-8 font-light",
                    "w-full md:w-48 transition duration-500 hover:bg-mint hover:text-black mt-12 tracking-wide"
                )}
                onClick={() => setShow(true)}
            >
                <div className="transform translate-y-0.5">{text}</div>
            </button>
        </>
    )
}

export default TypeformButton

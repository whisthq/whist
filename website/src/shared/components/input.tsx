import React, { ChangeEvent, KeyboardEvent } from "react"

const Input = (props: {
    onChange?: (evt: ChangeEvent<HTMLInputElement>) => void
    onKeyPress?: (evt: KeyboardEvent) => void
    placeholder: string
    text?: string
    type: string
    valid?: boolean
    value?: string
    warning?: string
    altText?: JSX.Element
    id?: string
}) => {
    return (
        <div style={{ width: "100%" }}>
            <div
                style={{
                    width: "100%",
                    display: "flex",
                    justifyContent: "space-between",
                }}
            >
                <div
                    style={{
                        fontSize: 14,
                        fontWeight: "bold",
                        color: "#333333",
                    }}
                >
                    {props.text}
                </div>
                <div
                    style={{
                        fontSize: 14,
                    }}
                >
                    {props.altText}
                </div>
                {props.warning && props.warning !== "" && (
                    <div
                        style={{
                            color: "white",
                            background: "#fc3d03",
                            fontSize: 12,
                            padding: "3px 25px",
                        }}
                    >
                        {props.warning}
                    </div>
                )}
            </div>
            <input
                id={props.id}
                type={props.type}
                aria-label="Default"
                aria-describedby="inputGroup-sizing-default"
                placeholder={props.placeholder}
                className="w-full bg-white border-gray-100 px-4 py-3 outline-none rounded-none text-black"
                onChange={props.onChange}
                onKeyPress={props.onKeyPress}
                value={props.value}
                style={{
                    marginTop: 12,
                    border: props.warning
                        ? "solid 1px #fc3d03"
                        : props.valid
                        ? "solid 1px rgba(223, 242, 224, 0.5)"
                        : "solid 1px #dfdfdf",
                    background: props.valid
                        ? "rgba(223, 242, 224, 0.5)"
                        : "#ffffff",
                }}
            />
        </div>
    )
}

export default Input

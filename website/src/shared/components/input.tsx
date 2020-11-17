import React from "react"
//import { FormControl } from "react-bootstrap"

const Input = (props: any) => {
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
                type={props.type}
                aria-label="Default"
                aria-describedby="inputGroup-sizing-default"
                placeholder={props.placeholder}
                className="input-form"
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

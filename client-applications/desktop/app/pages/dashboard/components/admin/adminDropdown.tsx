import React, { useState } from "react"
import { Form, Button, Dropdown } from "react-bootstrap"
import {
    FractalRegions,
    FractalWebservers,
    FractalTaskDefs,
    FractalClusters,
} from "pages/dashboard/components/admin/dropdownValues"

export const AdminDropdown = (props: {
    // how to modify the state
    onClick: (value: string | null) => void
    // it will display these plus a text box plus a reset button
    // the reset button will set the value to the defaultValue
    options: { [key: string]: string } // hacky
    defaultValue: string | null
    // value is what will be the dropdown value to be selected at this moment
    value: string | null
    // title will basically say what type it is i.e. webserver or cluster or etc...
    title: string
}) => {
    const { onClick, options, defaultValue, value, title } = props

    const [customValue, setCustomValue] = useState("")
    const validCustomValue = customValue.trim() !== "" && customValue.length > 3

    const changeCustomValue = (evt: any) => {
        setCustomValue(evt.target.value)
    }

    const handleDropdownClick = (option: string) => {
        if (
            option === FractalClusters.RESET ||
            option === FractalRegions.RESET ||
            option === FractalTaskDefs.RESET ||
            option === FractalWebservers.RESET
        ) {
            onClick(defaultValue)
        } else {
            onClick(option)
        }
    }

    const handleCustomClick = () => {
        if (validCustomValue) {
            onClick(customValue)
            setCustomValue("") // it will disable and avoid misclicks
        }
    }

    return (
        <div style={{ marginTop: 20, width: "200%" }}>
            <Dropdown>
                <Dropdown.Toggle variant="primary" id={title}>
                    {title}
                </Dropdown.Toggle>

                <Dropdown.Menu>
                    {Object.values(options).map((option: string) => (
                        <Dropdown.Item
                            key={option}
                            active={option === value}
                            onClick={() => handleDropdownClick(option)}
                        >
                            {option}
                        </Dropdown.Item>
                    ))}
                </Dropdown.Menu>
            </Dropdown>
            <Form>
                <Form.Group controlId={`${title}CustomValue`}>
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

export default AdminDropdown

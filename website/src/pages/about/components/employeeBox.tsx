import React from "react";
import { Col } from "react-bootstrap";
import Popup from "reactjs-popup";

const EmployeeBox = (props: { image: string; name: string; text: string }) => {
  return (
    <Col md={4} style={{ marginBottom: 20 }}>
      <Popup
        modal
        trigger={
          <div
            className="expandOnHover"
            style={{
              borderRadius: 5,
              padding: "40px 30px",
              textAlign: "center",
              background: "rgba(213, 225, 245, 0.2)",
              marginBottom: 40,
            }}
          >
            <img
              src={props.image}
              style={{
                maxWidth: 130,
                maxHeight: 130,
                border: "solid 8px white",
                borderRadius: "50%",
                margin: "auto",
              }}
              alt=""
            />
          </div>
        }
        contentStyle={{
          width: 500,
          borderRadius: 5,
          backgroundColor: "white",
          border: "none",
          minHeight: 325,
          padding: "30px 50px",
          boxShadow: "0px 4px 30px rgba(0,0,0,0.1)",
        }}
      >
        <div>
          <div
            style={{
              display: "flex",
            }}
          >
            <img
              src={props.image}
              style={{
                maxWidth: 75,
                maxHeight: 75,
              }}
              alt=""
            />
            <div
              style={{
                paddingLeft: 50,
              }}
            >
              <div
                style={{
                  fontSize: 30,
                  fontWeight: "bold",
                }}
              >
                {props.name}
              </div>
              <div
                className="font-body"
                style={{
                  marginTop: 20,
                  color: "#555555",
                }}
              >
                {props.text}
              </div>
            </div>
          </div>
        </div>
      </Popup>
    </Col>
  );
};

export default EmployeeBox;

import React, { Component } from "react";
import * as geolib from "geolib";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons";

class DistanceBox extends Component {
  constructor(props: any) {
    super(props);
    this.state = { distance: 0, distancebar: 50 };
  }

  CalculateDistance = (public_ip: any) => {
    let component = this;
    const iplocation = require("iplocation").default;
    var options = {
      enableHighAccuracy: true,
      timeout: 10000,
    };
    var metersInMile = 1609.34;
    var dist;

    navigator.geolocation.getCurrentPosition(showPosition, showError, options);

    function showPosition(position: any) {
      if (public_ip && public_ip !== "") {
        iplocation(public_ip).then(function (res: any) {
          dist = geolib.getDistance(
            {
              latitude: position.coords.latitude,
              longitude: position.coords.longitude,
            },
            {
              latitude: res.latitude,
              longitude: res.longitude,
            }
          );
          dist = Math.round(dist / metersInMile);
          component.setState({
            distance: dist,
            distancebar: Math.min(Math.max((220 * dist) / 1000, 80), 220),
          });
        });
      }
    }

    function showError(error: any) {
      console.log(error);
    }
  };

  componentDidMount() {
    this.CalculateDistance(this.props.public_ip);
  }

  componentDidUpdate(_prevProps: any) {
    if (this.props.public_ip != "" && this.state.distance == 0) {
      this.CalculateDistance(this.props.public_ip);
    }
  }

  render() {
    let distanceBox;

    if (this.state.distance < 250) {
      distanceBox = (
        <div>
          <div
            style={{
              background: "none",
              border: "solid 1px #14a329",
              height: 6,
              width: 6,
              borderRadius: 4,
              display: "inline",
              float: "left",
              position: "relative",
              top: 5,
              marginRight: 13,
            }}
          ></div>
          <div
            style={{
              marginTop: 5,
              fontSize: 14,
              fontWeight: "bold",
            }}
          >
            {this.state.distance} Mi. Cloud PC Distance
          </div>
          <div
            style={{
              marginTop: 8,
              fontSize: 12,
              color: "#333333",
              paddingLeft: 20,
              lineHeight: 1.4,
            }}
          >
            You are close enough to your cloud PC to experience low-latency
            streaming.
          </div>
        </div>
      );
    } else if (this.state.distance < 500) {
      distanceBox = (
        <div>
          <div
            style={{
              background: "none",
              border: "solid 1px #f2a20c",
              height: 6,
              width: 6,
              borderRadius: 4,
              display: "inline",
              float: "left",
              position: "relative",
              top: 5,
              marginRight: 13,
            }}
          ></div>
          <div
            style={{
              marginTop: 5,
              fontSize: 14,
              fontWeight: "bold",
            }}
          >
            {this.state.distance} Mi. Cloud PC Distance
          </div>
          <div
            style={{
              marginTop: 8,
              fontSize: 12,
              color: "#333333",
              paddingLeft: 20,
              lineHeight: 1.4,
            }}
          >
            You may experience slightly higher latency due to your distance from
            your cloud PC.
          </div>
        </div>
      );
    } else {
      distanceBox = (
        <div>
          <div
            style={{
              background: "none",
              border: "solid 1px #d13628",
              height: 6,
              width: 6,
              borderRadius: 4,
              display: "inline",
              float: "left",
              position: "relative",
              top: 5,
              marginRight: 13,
            }}
          ></div>
          <div
            style={{
              marginTop: 5,
              fontSize: 14,
              fontWeight: "bold",
            }}
          >
            {this.state.distance} Mi. Cloud PC Distance
          </div>
          <div
            style={{
              marginTop: 8,
              fontSize: 12,
              color: "#333333",
              paddingLeft: 20,
              lineHeight: 1.4,
            }}
          >
            You may experience latency because you are far away from your cloud
            PC.
          </div>
        </div>
      );
    }

    return (
      <div style={{ marginTop: 20 }}>
        {this.state.distance === 0 ? (
          <div
            style={{
              marginTop: 25,
              position: "relative",
              right: 5,
            }}
          >
            <FontAwesomeIcon
              icon={faCircleNotch}
              spin
              style={{
                color: "#5EC4EB",
                height: 10,
                display: "inline",
                marginRight: 9,
              }}
            />
            <div
              style={{
                marginTop: 8,
                fontSize: 12,
                color: "#333333",
                display: "inline",
              }}
            >
              Checking current location
            </div>
          </div>
        ) : (
          <div>
            <div style={{ height: 10 }}></div>
            {distanceBox}
          </div>
        )}
      </div>
    );
  }
}

export default DistanceBox;

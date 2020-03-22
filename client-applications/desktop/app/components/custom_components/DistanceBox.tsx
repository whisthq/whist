import React, { Component } from 'react';
import * as geolib from 'geolib';

class DistanceBox extends Component {
  constructor(props) {
    super(props)
    this.state = {distance: 0, distancebar: 50}
  }

  CalculateDistance = (public_ip) => {
    let component = this;
    const iplocation = require("iplocation").default;
    var options = {
      enableHighAccuracy: true,
      timeout: 10000
    }
    var metersInMile = 1609.34;
    var dist;

    navigator.geolocation.getCurrentPosition(showPosition, showError, options);

    function showPosition(position) {
      iplocation(public_ip).then(function(res) {
        dist = geolib.getDistance(
        {
          latitude: position.coords.latitude,
          longitude: position.coords.longitude
        },
        {
          latitude: res.latitude,
          longitude: res.longitude,
        });
        dist = Math.round(dist / metersInMile)
        component.setState({distance: dist,
          distancebar: Math.min(Math.max(220 * dist / 1000, 80), 220)})
      })
    }

    function showError(error) {
    }
  }

  componentDidMount() {
    this.CalculateDistance(this.props.public_ip)
  }

  componentDidUpdate(prevProps) {
    if(this.props.public_ip != null && this.state.distance == 0) {
      this.CalculateDistance(this.props.public_ip)
    }
  }


  render() {
    let distanceBox;
    
    if(this.state.distance < 250) {
      distanceBox =
        <div>
         <div style = {{background: "none", border: "solid 1px #3ce655", height: 6, width: 6, borderRadius: 3, borderRadius: 4, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 7}}>
         </div>
          <div style = {{marginTop: 5, fontSize: 14, fontWeight: "bold"}}>
            Cloud PC Distance: <span style = {{color: "#5EC4EB", fontWeight: "bold"}}> {this.state.distance} mi </span>
          </div>
          <div style = {{marginTop: 8, fontSize: 11, color: "#D6D6D6"}}>
            You are close enough to your cloud PC to experience low-latency streaming.
          </div>
        </div>
    } else if(this.state.distance < 500) {
      distanceBox =
        <div>
         <div style = {{background: "none", border: "solid 1px #f2a20c", height: 6, width: 6, borderRadius: 3, borderRadius: 4, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 7}}>
         </div>
          <div style = {{marginTop: 5, fontSize: 14, fontWeight: "bold"}}>
            Cloud PC Distance: <span style = {{color: "#5EC4EB", fontWeight: "bold"}}> {this.state.distance} mi </span>
          </div>
          <div style = {{marginTop: 8, fontSize: 11, color: "#D6D6D6"}}>
            You may experience slightly higher latency due to your distance from your cloud PC.
          </div>
        </div>
    } else {
      distanceBox =
        <div>
         <div style = {{background: "none", border: "solid 1px #d13628", height: 6, width: 6, borderRadius: 3, borderRadius: 4, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 7}}>
         </div>
          <div style = {{marginTop: 5, fontSize: 14, fontWeight: "bold"}}>
            Cloud PC Distance: <span style = {{color: "#5EC4EB", fontWeight: "bold"}}> {this.state.distance} mi </span>
          </div>
          <div style = {{marginTop: 8, fontSize: 11, color: "#D6D6D6"}}>
            You may experience latency because you are far away from your cloud PC.
          </div>
        </div>
    }

    return (
      <div style = {{marginTop: 35}}>
        {
          this.state.distance === 0
          ?
          <div>
            <div style = {{width: 220, height: `${ this.props.barHeight }px`, borderRadius: "0px 2px 2px 0px", background: "#111111"}}>
            </div>
            <div style = {{marginTop: 8, fontSize: 11, color: "white"}}>
              <div style = {{background: "none", border: "solid 1px #5EC4EB", height: 6, width: 6, borderRadius: 3, display: "inline", float: "left", position: 'relative', top: 3, marginRight: 7}}>
              </div>
              Checking current location
            </div>
          </div>
          :
          <div>
            <div style = {{width: 220, height: `${ this.props.barHeight }px`, borderRadius: "0px 2px 2px 0px", background: "#111111", position: "absolute", zIndex: 0}}>
            </div>
            <div style = {{width: `${ this.state.distancebar }px`, height: `${ this.props.barHeight }px`, borderRadius: "0px 2px 2px 0px", background: "#5EC4EB", position: "absolute", zIndex: 1}}>
            </div>
            <div style = {{height: 10}}></div>
            {distanceBox}
          </div>
        }
      </div>
    )
  }
}

export default DistanceBox
// This file is required by the index.html file and will
// be executed in the renderer process for that window.
// All of the Node.js APIs are available in this process.
const remote      = require('electron').remote
const { shell }   = require('electron')
const geolib      = require('geolib')
var speedTest     = require('speedtest-net')
var internetspeed = speedTest({maxTime: 2500})
var geoip         = require('geoip-lite')
var child         = require('child_process').spawn;
var latitude      = 38.676233
var longitude     = -78.156443


document.getElementById("internet-speed").innerHTML = '<img src = "assets/spinner.svg" width = "15px" class = "fa-spin spinner"/><span class = "notification-text">Checking Internet speed</span>'
document.getElementById("location").innerHTML = '<img src = "assets/spinner.svg" width = "15px" class = "fa-spin spinner"/><span class = "notification-text">Checking cloud distance</span>'

internetspeed.on('data', data => {
  var speed = Math.round(data.speeds.download)
  if(speed < 15) {
    document.getElementById("internet-speed").innerHTML = 'Internet speed: ' +
    '<span style = "color: #C02E2E">Slow (' + speed + ' Mbps)</span>' +
    '<div class = "internet-speed-text">You may experience lag or blurry images. \
    For the best experience, we recommend Internet speeds of at least 15 Mbps. \
    Try closing other bandwidth-consuming apps, like YouTube, Netflix, or Spotify.</div>'
  } else if (speed < 25) {
    document.getElementById("internet-speed").innerHTML = 'Internet speed: ' +
    '<span style = "color: #FEC14B">Medium (' + speed + ' Mbps)</span>' +
    '<div class = "internet-speed-text">Expect a smooth streaming experience, although dips in Internet speed below 15 Mbps \
    could cause latency. Try closing other bandwidth-consuming apps, like YouTube, Netflix, or Spotify.</div>'
  } else if (speed < 35) {
    document.getElementById("internet-speed").innerHTML = 'Internet speed: ' +
    '<span style = "color: #36E251">Fast (' + speed + ' Mbps)</span>' +
    '<div class = "internet-speed-text">Your Internet is fast enough to support latency-free streaming.</div>'
  } else {
    document.getElementById("internet-speed").innerHTML = 'Internet speed: ' +
    '<span style = "color: #36E251">Excellent (' + speed + ' Mbps)</span>' +
    '<div class = "internet-speed-text">Your Internet is fast enough to support latency-free streaming.</div>'
  }
});

internetspeed.on('error', err => {
  document.getElementById("internet-speed").innerHTML = '<span style = "font-family: Maven Pro; font-size: 12px">Could not detect internet speed</span>'
});


function getUserIP(onNewIP) { //  onNewIp - your listener function for new IPs
    //compatibility for firefox and chrome
    var myPeerConnection = window.RTCPeerConnection || window.mozRTCPeerConnection || window.webkitRTCPeerConnection;
    var pc = new myPeerConnection({
        iceServers: []
    }),
    noop = function() {},
    localIPs = {},
    ipRegex = /([0-9]{1,3}(\.[0-9]{1,3}){3}|[a-f0-9]{1,4}(:[a-f0-9]{1,4}){7})/g,
    key;

    function iterateIP(ip) {
        if (!localIPs[ip]) onNewIP(ip);
        localIPs[ip] = true;
    }

     //create a bogus data channel
    pc.createDataChannel("");

    // create offer and set local description
    pc.createOffer().then(function(sdp) {
        sdp.sdp.split('\n').forEach(function(line) {
            if (line.indexOf('candidate') < 0) return;
            line.match(ipRegex).forEach(iterateIP);
        });

        pc.setLocalDescription(sdp, noop, noop);
    }).catch(function(reason) {
        // An error occurred, so handle the failure to connect
    });

    //listen for candidate events
    pc.onicecandidate = function(ice) {
        if (!ice || !ice.candidate || !ice.candidate.candidate || !ice.candidate.candidate.match(ipRegex)) return;
        ice.candidate.candidate.match(ipRegex).forEach(iterateIP);
    };
}

try {
    navigator.geolocation.getCurrentPosition(
        function(position) {
        },
        () => {
            getUserIP(function(ip){
                var geo = geoip.lookup(ip);
                var distance = geolib.getDistance(
                    {latitude: geo.ll[0], longitude: geo.ll[1]},
                    {latitude: latitude, longitude: longitude}
                ) / 1609.34
                distance = Math.round(distance)
                if(distance < 250) {
                    document.getElementById("location").innerHTML = 'Cloud Distance: ' +
                    '<span style = "color: #36E251">Excellent (' + distance + ' miles)</span>' +
                    '<div class = "internet-speed-text">You are within close range of your cloud PC.</div>'
                } else if (distance < 500) {
                    document.getElementById("location").innerHTML = 'Cloud Distance: ' +
                    '<span style = "color: #FEC14B">Medium (' + distance + ' miles)</span>' +
                    '<div class = "internet-speed-text">You may experience slightly higher latency due to your distance from your cloud PC.</div>'
                } else {
                    document.getElementById("location").innerHTML = 'Cloud Distance: ' +
                    '<span style = "color: #C02E2E">Far (' + distance + ' miles)</span>' +
                    '<div class = "internet-speed-text">You may experience high latency because you are far away from your cloud PC.</div>'
                }
            });
        }
    );
} catch(err) {
    document.getElementById("location").innerHTML = '<span style = "font-family: Maven Pro; font-size: 12px">Could not detect current location</span>'
}

// When document has loaded, initialise
document.onreadystatechange = () => {
    if (document.readyState == "complete") {
        handleWindowControls();
    }
};

document.querySelector('#signup-button').addEventListener('click', () => {
    signupRedirect();
})

document.querySelector('#forgot-button').addEventListener('click', () => {
    passwordRedirect();
})


document.querySelector('#login-button').addEventListener('click', () => {
    document.getElementById('login-button').disabled = true
    document.getElementById('login-button').opacity = 0.7
    var username = document.getElementById('username').value
    var password = document.getElementById('password').value
    logonUser(username, password)
})

function handleWindowControls() {

    let win = remote.getCurrentWindow();
    // Make minimise/maximise/restore/close buttons work when they are clicked
    document.getElementById('min-button').addEventListener("click", event => {
        win.minimize();
    });

    document.getElementById('restore-button').addEventListener("click", event => {
        win.unmaximize();
    });

    document.getElementById('close-button').addEventListener("click", event => {
        win.close();
    });

    // Toggle maximise/restore buttons when maximisation/unmaximisation occurs
    toggleMaxRestoreButtons();
    win.on('maximize', toggleMaxRestoreButtons);
    win.on('unmaximize', toggleMaxRestoreButtons);

    function toggleMaxRestoreButtons() {
        if (win.isMaximized()) {
            document.body.classList.add('maximized');
        } else {
            document.body.classList.remove('maximized');
        }
    }
}

function signupRedirect() {
    shell.openExternal('https://www.fractalcomputers.com/auth');
}

function passwordRedirect() {
    shell.openExternal('https://www.fractalcomputers.com/reset');
}

function logonUser(username, password) {
    const url = 'https://cube-celery-vm.herokuapp.com/user/login';
    const body = {
        username: username,
        password: password
    }

    var xhr = new XMLHttpRequest();

    xhr.onreadystatechange = function () {
        if (this.readyState != 4) return;

        if (this.status == 200) {
            var data = JSON.parse(this.responseText)
            runProtocol(data.public_ip, 1);
        } else {
            document.getElementById('login-button').disabled = false
            document.getElementById('login-button').opacity = 1.0
            document.getElementById("warning").innerHTML = 'Invalid credentials. Your password can be recovered from our website.'
            document.getElementById('password').value = ''
        }
    };

    xhr.open("POST", url, true);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send(JSON.stringify(body));
}

function runProtocol(public_ip, tries) {
    // var executablePath = process.cwd() + "\\fractal-protocol\\desktop\\desktop.exe"
    var executablePath = process.cwd() + "/fractal-protocol/desktop/desktop"
    var parameters = [public_ip]
    document.getElementById("warning").innerHTML = '<img src = "assets/spinner.svg" width = "14px" class = "fa-spin spinner" style = "margin-right: 5px"/>Starting to stream'

    child(executablePath, parameters, {detached: true, stdio: 'ignore'});

    document.getElementById('password').value = ''
    document.getElementById('login-button').disabled = false
    document.getElementById('login-button').opacity = 1.0
    document.getElementById("warning").innerHTML = ''

    return 1
}

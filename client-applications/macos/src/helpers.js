// var th = ['','thousand','million', 'billion','trillion'];
// uncomment this line for English Number System
// var th = ['','thousand','million', 'milliard','billion'];

// var a = ['','One ','Two ','Three ','Four ', 'Five ','Six ','Seven ','Eight ','Nine ','Ten ','Eleven ','Twelve ','Thirteen ','Fourteen ','Fifteen ','Sixteen ','Seventeen ','Eighteen ','Nineteen '];
// var b = ['', '', 'Twenty','Thirty','Forty','Fifty', 'Sixty','Seventy','Eighty','Ninety'];

// function inWord (num) {
//     if ((num = num.toString()).length > 9) return 'overflow';
//     n = ('000000000' + num).substr(-9).match(/^(\d{2})(\d{2})(\d{2})(\d{1})(\d{2})$/);
//     if (!n) return; var str = '';
//     str += (n[1] != 0) ? (a[Number(n[1])] || b[n[1][0]] + ' ' + a[n[1][1]]) + 'Crore ' : '';
//     str += (n[2] != 0) ? (a[Number(n[2])] || b[n[2][0]] + ' ' + a[n[2][1]]) + 'Lakh ' : '';
//     str += (n[3] != 0) ? (a[Number(n[3])] || b[n[3][0]] + ' ' + a[n[3][1]]) + 'Thousand ' : '';
//     str += (n[4] != 0) ? (a[Number(n[4])] || b[n[4][0]] + ' ' + a[n[4][1]]) + 'Hundred ' : '';
//     str += (n[5] != 0) ? ((str != '') ? 'and ' : '') + (a[Number(n[5])] || b[n[5][0]] + ' ' + a[n[5][1]]) : '';
//     return str;
// }

// try {
//     var cores = navigator.hardwareConcurrency;
//     if(cores < 4) {
//         document.getElementById("cores").innerHTML = 'Logical Cores: ' + 
//         '<span style = "color: #C02E2E; font-weight: bold">' + inWord(cores) + '</span>' +
//         '<div class = "internet-speed-text">Because Fractal consumes four logical cores, you may experience crashing/stuttering.</div>' 
//     } else if (cores == 4) {
//         document.getElementById("cores").innerHTML = 'Logical Cores: ' + 
//         '<span style = "color: #FEC14B; font-weight: bold">' + inWord(cores) + '</span>' +
//         '<div class = "internet-speed-text">Fractal will utilize most of your CPU. For best performance, close other local apps.</div>' 
//     } else {
//         document.getElementById("cores").innerHTML = 'Logical Cores: ' + 
//         '<span style = "color: #36E251; font-weight: bold">' + inWord(cores) + '</span>' +
//         '<div class = "internet-speed-text">Your CPU has a sufficient logical core count.</div>' 
//     }
// } 
// catch(err) {
//     document.getElementById("cores").innerHTML = '<span style = "font-family: Maven Pro; font-size: 12px">Could not detect core count</span>'
// }
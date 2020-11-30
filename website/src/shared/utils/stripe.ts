export const getZipState = (zipcode: string) => {
    // Ensure we don't parse strings starting with 0 as octal values
    const code = parseInt(zipcode, 10)

    // Code blocks alphabetized by state
    if (code >= 35000 && code <= 36999) {
        return "AL"
    } else if (code >= 99500 && code <= 99999) {
        return "AK"
    } else if (code >= 85000 && code <= 86999) {
        return "AZ"
    } else if (code >= 71600 && code <= 72999) {
        return "AR"
    } else if (code >= 90000 && code <= 96699) {
        return "CA"
    } else if (code >= 80000 && code <= 81999) {
        return "CO"
    } else if (code >= 6000 && code <= 6999) {
        return "CT"
    } else if (code >= 19700 && code <= 19999) {
        return "DE"
    } else if (code >= 32000 && code <= 34999) {
        return "FL"
    } else if (code >= 30000 && code <= 31999) {
        return "GA"
    } else if (code >= 96700 && code <= 96999) {
        return "HI"
    } else if (code >= 83200 && code <= 83999) {
        return "ID"
    } else if (code >= 60000 && code <= 62999) {
        return "IL"
    } else if (code >= 46000 && code <= 47999) {
        return "IN"
    } else if (code >= 50000 && code <= 52999) {
        return "IA"
    } else if (code >= 66000 && code <= 67999) {
        return "KS"
    } else if (code >= 40000 && code <= 42999) {
        return "KY"
    } else if (code >= 70000 && code <= 71599) {
        return "LA"
    } else if (code >= 3900 && code <= 4999) {
        return "ME"
    } else if (code >= 20600 && code <= 21999) {
        return "MD"
    } else if (code >= 1000 && code <= 2799) {
        return "MA"
    } else if (code >= 48000 && code <= 49999) {
        return "MI"
    } else if (code >= 55000 && code <= 56999) {
        return "MN"
    } else if (code >= 38600 && code <= 39999) {
        return "MS"
    } else if (code >= 63000 && code <= 65999) {
        return "MO"
    } else if (code >= 59000 && code <= 59999) {
        return "MT"
    } else if (code >= 27000 && code <= 28999) {
        return "NC"
    } else if (code >= 58000 && code <= 58999) {
        return "ND"
    } else if (code >= 68000 && code <= 69999) {
        return "NE"
    } else if (code >= 88900 && code <= 89999) {
        return "NV"
    } else if (code >= 3000 && code <= 3899) {
        return "NH"
    } else if (code >= 7000 && code <= 8999) {
        return "NJ"
    } else if (code >= 87000 && code <= 88499) {
        return "NM"
    } else if (code >= 10000 && code <= 14999) {
        return "NY"
    } else if (code >= 43000 && code <= 45999) {
        return "OH"
    } else if (code >= 73000 && code <= 74999) {
        return "OK"
    } else if (code >= 97000 && code <= 97999) {
        return "OR"
    } else if (code >= 15000 && code <= 19699) {
        return "PA"
    } else if (code >= 300 && code <= 999) {
        return "PR"
    } else if (code >= 2800 && code <= 2999) {
        return "RI"
    } else if (code >= 29000 && code <= 29999) {
        return "SC"
    } else if (code >= 57000 && code <= 57999) {
        return "SD"
    } else if (code >= 37000 && code <= 38599) {
        return "TN"
    } else if (
        (code >= 75000 && code <= 79999) ||
        (code >= 88500 && code <= 88599)
    ) {
        return "TX"
    } else if (code >= 84000 && code <= 84999) {
        return "UT"
    } else if (code >= 5000 && code <= 5999) {
        return "VT"
    } else if (code >= 22000 && code <= 24699) {
        return "VA"
    } else if (code >= 20000 && code <= 20599) {
        return "DC"
    } else if (code >= 98000 && code <= 99499) {
        return "WA"
    } else if (code >= 24700 && code <= 26999) {
        return "WV"
    } else if (code >= 53000 && code <= 54999) {
        return "WI"
    } else if (code >= 82000 && code <= 83199) {
        return "WY"
    } else {
        return null
    }
}

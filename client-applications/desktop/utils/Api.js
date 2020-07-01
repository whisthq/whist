export function apiPost(endpoint, body, token) {
    return fetch(endpoint, {
        method: "POST",
        mode: "cors",
        headers: {
            "Content-Type": "application/json",
            Authorization: "Bearer " + token,
        },
        body: JSON.stringify(body),
    }).then((response) => {
        return response.json().then((json) => ({ json, response }));
    });
}

export function apiGet(endpoint, token) {
    return fetch(endpoint, {
        method: "GET",
        mode: "cors",
        headers: {
            "Content-Type": "application/json",
            Authorization: "Bearer " + token,
        },
    }).then((response) => {
        return response.json().then((json) => ({ json, response }));
    });
}

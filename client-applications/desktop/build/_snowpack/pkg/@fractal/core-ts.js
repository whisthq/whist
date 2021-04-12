import { c as createCommonjsModule, g as getDefaultExportFromNamespaceIfNotNamed, a as commonjsGlobal } from '../common/_commonjsHelpers-798ad6a7.js';
import { p as process } from '../common/process-2545f00a.js';
import { l as lodash } from '../common/lodash-69ed1375.js';

var api = createCommonjsModule(function (module, exports) {
Object.defineProperty(exports, "__esModule", { value: true });
exports.FractalHTTPContent = exports.FractalHTTPRequest = exports.FractalHTTPCode = exports.FractalAPI = void 0;
class FractalAPI {
}
exports.FractalAPI = FractalAPI;
FractalAPI.TOKEN = {
    REFRESH: "/token/refresh",
    VALIDATE: "/token/validate",
};
FractalAPI.CONTAINER = {
    CREATE: "/container/create",
    ASSIGN: "/container/assign",
    TEST_CREATE: "/aws_container/create_container",
};
FractalAPI.MAIL = {
    FEEDBACK: "/mail/feedback",
};
FractalAPI.ACCOUNT = {
    LOGIN: "/account/login",
};
(function (FractalHTTPCode) {
    FractalHTTPCode[FractalHTTPCode["SUCCESS"] = 200] = "SUCCESS";
    FractalHTTPCode[FractalHTTPCode["ACCEPTED"] = 202] = "ACCEPTED";
    FractalHTTPCode[FractalHTTPCode["PARTIAL_CONTENT"] = 206] = "PARTIAL_CONTENT";
    FractalHTTPCode[FractalHTTPCode["BAD_REQUEST"] = 400] = "BAD_REQUEST";
    FractalHTTPCode[FractalHTTPCode["UNAUTHORIZED"] = 401] = "UNAUTHORIZED";
    FractalHTTPCode[FractalHTTPCode["PAYMENT_REQUIRED"] = 402] = "PAYMENT_REQUIRED";
    FractalHTTPCode[FractalHTTPCode["FORBIDDEN"] = 403] = "FORBIDDEN";
    FractalHTTPCode[FractalHTTPCode["NOT_FOUND"] = 404] = "NOT_FOUND";
    FractalHTTPCode[FractalHTTPCode["METHOD_NOT_ALLOWED"] = 405] = "METHOD_NOT_ALLOWED";
    FractalHTTPCode[FractalHTTPCode["NOT_ACCEPTABLE"] = 406] = "NOT_ACCEPTABLE";
    FractalHTTPCode[FractalHTTPCode["REQUEST_TIMEOUT"] = 408] = "REQUEST_TIMEOUT";
    FractalHTTPCode[FractalHTTPCode["CONFLICT"] = 409] = "CONFLICT";
    FractalHTTPCode[FractalHTTPCode["INTERNAL_SERVER_ERROR"] = 500] = "INTERNAL_SERVER_ERROR";
})(exports.FractalHTTPCode || (exports.FractalHTTPCode = {}));
(function (FractalHTTPRequest) {
    FractalHTTPRequest["POST"] = "POST";
    FractalHTTPRequest["GET"] = "GET";
    FractalHTTPRequest["PUT"] = "PUT";
    FractalHTTPRequest["DELETE"] = "DELETE";
})(exports.FractalHTTPRequest || (exports.FractalHTTPRequest = {}));
(function (FractalHTTPContent) {
    FractalHTTPContent["JSON"] = "application/json";
    FractalHTTPContent["TEXT"] = "application/text";
})(exports.FractalHTTPContent || (exports.FractalHTTPContent = {}));
});

var lib = createCommonjsModule(function (module, exports) {

Object.defineProperty(exports, "__esModule", {
  value: true
});

var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; };

/* global window self */

var isBrowser = typeof window !== 'undefined' && typeof window.document !== 'undefined';

/* eslint-disable no-restricted-globals */
var isWebWorker = (typeof self === 'undefined' ? 'undefined' : _typeof(self)) === 'object' && self.constructor && self.constructor.name === 'DedicatedWorkerGlobalScope';
/* eslint-enable no-restricted-globals */

var isNode = typeof process !== 'undefined' && process.versions != null && process.versions.node != null;

/**
 * @see https://github.com/jsdom/jsdom/releases/tag/12.0.0
 * @see https://github.com/jsdom/jsdom/issues/1537
 */
/* eslint-disable no-undef */
var isJsDom = function isJsDom() {
  return typeof window !== 'undefined' && window.name === 'nodejs' || navigator.userAgent.includes('Node.js') || navigator.userAgent.includes('jsdom');
};

exports.isBrowser = isBrowser;
exports.isWebWorker = isWebWorker;
exports.isNode = isNode;
exports.isJsDom = isJsDom;
});

// native patch for: node-fetch, whatwg-fetch
// ref: https://github.com/tc39/proposal-global
var getGlobal = function () {
  if (typeof self !== 'undefined') { return self; }
  if (typeof window !== 'undefined') { return window; }
  if (typeof global !== 'undefined') { return global; }
  throw new Error('unable to locate global object');
};
var global = getGlobal();
var nodeFetch = global.fetch.bind(global);
const Headers = global.Headers;
const Request = global.Request;
const Response = global.Response;

var nodeFetch$1 = /*#__PURE__*/Object.freeze({
    __proto__: null,
    'default': nodeFetch,
    Headers: Headers,
    Request: Request,
    Response: Response
});

var require$$0 = /*@__PURE__*/getDefaultExportFromNamespaceIfNotNamed(nodeFetch$1);

var fetchBase_1 = createCommonjsModule(function (module, exports) {
var __importDefault = (commonjsGlobal && commonjsGlobal.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.fetchBase = void 0;


const node_fetch_1 = __importDefault(require$$0);
// TODO: I don't think fractalBackoff() actually works as it's supposed to.
// Here's a way we can fix: https://www.npmjs.com/package/fetch-retry
/*
 * Performs an HTTP request. Must be passed url and method
 * keys. Not intended for direct usage, but as a base function
 * to be wrapped in a decorator pipeline.
 *
 * The request object is contained within the return value.
 *
 * @param req - a ServerRequest object
 * @returns a ServerResponse wrapped in a Promise
 */
const fetchBase = async (req) => {
    const fetchFunc = lib.isNode ? node_fetch_1.default : fetch;
    const response = await fetchFunc(req.url || "", {
        method: req.method || "",
        // mode: "cors",
        headers: {
            "Content-Type": api.FractalHTTPContent.JSON,
            Authorization: `Bearer ${req.token}`,
        },
        body: JSON.stringify(req.body),
    });
    return { request: req, response };
};
exports.fetchBase = fetchBase;
});

var withCatch_1 = createCommonjsModule(function (module, exports) {
Object.defineProperty(exports, "__esModule", { value: true });
exports.withCatch = void 0;
/*
 * Catches any errors downstream in a decorator pipeline.
 * Caught erorrs will be returned upstream on the error key.
 *
 * @param fn - The downstream decorator function
 * @param req - a ServerRequest object
 * @returns a ServerResponse wrapped in a Promise
 */
const withCatch = async (fn, req) => {
    try {
        return await fn(req);
    }
    catch (err) {
        return {
            request: req,
            error: err,
            response: { status: 500 },
        };
    }
};
exports.withCatch = withCatch;
});

var withGet_1 = createCommonjsModule(function (module, exports) {
Object.defineProperty(exports, "__esModule", { value: true });
exports.withGet = void 0;
/*
 * Attaches a GET method to a ServerRequest.
 *
 * @param fn - The downstream decorator function
 * @param req - a ServerRequest object
 * @returns a ServerResponse wrapped in a Promise
 */
const withGet = async (fn, req) => await fn({ ...req, method: "GET" });
exports.withGet = withGet;
});

var withJSON_1 = createCommonjsModule(function (module, exports) {
Object.defineProperty(exports, "__esModule", { value: true });
exports.withJSON = void 0;
/*
 * Awaits a JSON stream from the server, and passes the result
 * upstream in a decorator pipeline.
 *
 * @param fn - The downstream decorator function
 * @param req - a ServerRequest object
 * @returns a Promise with the HTTP response
 */
const withJSON = async (fn, req) => {
    const { response } = await fn(req);
    // Certain badly formed responses, like internal server 500,
    // will throw an error if you call the response.json() method.
    // This can happen even though a json method exists on the
    // response object.
    //
    // This is a unusual case, so we'll choose to catch the error
    // and simply leave the json key unassigned when passing the
    // result upstream.
    //
    // The json method is still available on the response key,
    // which is holding the original response object, in case
    // the user wants to try for themselves.
    if (response === null || response === void 0 ? void 0 : response.json)
        try {
            return { response, json: await response.json() };
        }
        catch (e) { }
    return { response };
};
exports.withJSON = withJSON;
});

var withPost_1 = createCommonjsModule(function (module, exports) {
Object.defineProperty(exports, "__esModule", { value: true });
exports.withPost = void 0;
/*
 * Attaches a POST method to a ServerRequest.
 *
 * @param fn - The downstream decorator function
 * @param req - a ServerRequest object
 * @returns a ServerResponse wrapped in a Promise
 */
const withPost = async (fn, req) => await fn({ ...req, method: "POST" });
exports.withPost = withPost;
});

var withStatus_1 = createCommonjsModule(function (module, exports) {
Object.defineProperty(exports, "__esModule", { value: true });
exports.withStatus = void 0;
/*
 * Adds 'status' and 'statusText' keys to the
 * top-level response object. Returns the rest
 * of the ServerResponse unchanged.
 *
 * @param fn - The downstream decorator function
 * @param req - a ServerRequest object
 * @returns a ServerResponse wrapped in a Promise
 */
const withStatus = async (fn, req) => {
    var _a, _b;
    const result = await fn(req);
    return {
        ...result,
        status: (_a = result.response) === null || _a === void 0 ? void 0 : _a.status,
        statusText: (_b = result.response) === null || _b === void 0 ? void 0 : _b.statusText,
    };
};
exports.withStatus = withStatus;
});

var withServer_1 = createCommonjsModule(function (module, exports) {
Object.defineProperty(exports, "__esModule", { value: true });
exports.withServer = void 0;
/*
 * Accepts a "server" parameter at configuration time.
 * Returns a decorator that attaches a server to a ServerRequest.
 *
 * @param server - a string representing a HTTP server
 * @returns a ServerDecorator function
 */
const withServer = (server) => async (fn, req) => await fn({ ...req, server });
exports.withServer = withServer;
});

var withURL_1 = createCommonjsModule(function (module, exports) {
Object.defineProperty(exports, "__esModule", { value: true });
exports.withURL = void 0;

/*
 * Builds a url using a server string and an endpoint string.
 *
 * @param server - a string representing a HTTP server
 * @param endpoint - a string representing a HTTP endpoint
 * @returns a string representing a HTTP request URL
 */
const serverUrl = (server, endpoint) => `${server}${endpoint}`;
/*
 * Ensures that a url is present on a ServerRequest object.
 * If a url is present, it passes on the object unchanged.
 * If no url is present, it attempts to build one using the
 * server and endpoint keys.
 * Without valid url, server, or endpoint keys, it will
 * stop the HTTP request and return a error ServerResponse.
 *
 * @param fn - The downstream decorator function
 * @param req - a ServerRequest object
 * @returns a ServerResponse wrapped in a Promise
 */
const withURL = async (fn, req) => {
    const { url, server, endpoint } = req;
    if (lodash.isString(url)) {
        return await fn(req);
    }
    else {
        if (lodash.isString(server) && lodash.isString(endpoint)) {
            return await fn({
                ...req,
                url: serverUrl(server || "", endpoint || ""),
            });
        }
        else {
            return {
                request: req,
                error: Error("Server API not configured with: url or [server, endpoint]."),
            };
        }
    }
};
exports.withURL = withURL;
});

var errors = createCommonjsModule(function (module, exports) {
Object.defineProperty(exports, "__esModule", { value: true });
exports.USER_LOGGED_OUT = void 0;
exports.USER_LOGGED_OUT = "USER_LOGGED_OUT";
});

var utilities = createCommonjsModule(function (module, exports) {
Object.defineProperty(exports, "__esModule", { value: true });
exports.decorate = exports.decorateDebug = void 0;
/*
 * Returns a function that extras the element of an array
 * at the given index.
 *
 * @param index - a number
 * @returns a function to select the element by index
 */
const index = (idx) => {
    return function getIndex(arr) {
        return arr[idx];
    };
};
/*
 * A utility functon to get the length of an array.
 *
 * @param arr - arr to measure
 * @returns a number, the length of the array
 */
const lengths = (arr) => arr.length;
/*
 * A utility function to alternate items from arrays.
 *
 * Takes any number of arrays a, b, c, ... and returns an array
 * which contains each of their arguments in alternated order, such
 * as [a1, b1, c1, ..., a2, b2, c2,...].
 *
 * @param { ...arrays } - arrays to interleave
 * @returns an array with the interleaved elements
 */
const interleave = (...arrays) => {
    let length = Math.min.apply(Math, arrays.map(lengths));
    let result = [];
    for (var idx = 0; idx < length; ++idx) {
        result = result.concat(arrays.map(index(idx)));
    }
    return result;
};
/*
 * A logging utility for decorated function pipelines.
 *
 * Can be used exactly like decorate, and will log to the console
 * the result at each step.
 *
 * The first log is always the initial arguments, and the last log
 * is the final result.
 *
 * @param func - a function to be wrapped by decorators
 * @param { ...decs } - decorators to wrap base function
 * @returns a function that pipelines func and { ...decs }
 */
const logDecorator = (logPrefix, inputFn, outputFn) => {
    return async (fn, ...args) => {
        console.log(logPrefix(...args), "in: ", inputFn(...args));
        const response = await fn(...args);
        console.log(logPrefix(response), "out:", outputFn(response));
        return response;
    };
};
/*
 * A logging utility for decorated function pipelines.
 *
 * Can be used exactly like decorate, and will log to the console
 * the result at each step.
 *
 * The first log is always the initial arguments, and the last log
 * is the final result.
 *
 * @param func - a function to be wrapped by decorators
 * @param { ...decs } - decorators to wrap base function
 * @returns a function that pipelines func and { ...decs }
 */
const decorateDebug = (fn, ...decs) => {
    let counter = 0;
    const prefix = () => {
        const msg = "log " + counter;
        counter++;
        return msg;
    };
    const inputFn = (x) => x;
    const outputFn = (x) => x;
    const initDbg = logDecorator(prefix, inputFn, outputFn);
    const decsDbg = logDecorator(prefix, inputFn, outputFn);
    interleave([], []);
    return decorate(fn, initDbg, ...interleave(decs, decs.map(() => decsDbg)));
};
exports.decorateDebug = decorateDebug;
/*
 * Composes higher-order decorator functions onto a base function.
 *
 * The resulting function pipelines the decorators in the order they
 * are passed, with the base function on the end. Input flows
 * "downstream" from left to right through the decorators and is then
 * passed to the base function. The return value of the base function
 * flows "upstream" from right to left back through the decorators.
 *
 * Each decorator function must accept a "continuation" as its first
 * argument, followed by the arguments to be passed to the base
 * function. The continuation function represents the "next" step in
 * the decorator pipeline, and must be called to continue the flow of
 * data downstream.
 *
 * A value must be returned by a decorator function to continue the
 * flow of data upstream.
 *
 * @param func - a function to be wrapped by decorators
 * @param { ...decs } - decorators to wrap base function
 * @returns a function that pipelines func and { ...decs }
 */
function decorate(func, ...decs) {
    return decs.reduceRight((d2, d1) => ((...args) => d1(d2, ...args)), func);
}
exports.decorate = decorate;
});

var withTokenRefresh_1 = createCommonjsModule(function (module, exports) {
Object.defineProperty(exports, "__esModule", { value: true });
exports.withTokenRefresh = void 0;






const post = utilities.decorate(fetchBase_1.fetchBase, withPost_1.withPost, withURL_1.withURL, withJSON_1.withJSON);
/*
 * Test for the shape of a refresh or access token.
 *
 * @param token - any type, a valid token is a string
 * @returns true if token shape is valid, false otherwise
 */
const isToken = (token) => (!token || token === "" ? false : true);
/*
 * Handles access to protected resources.
 * Caught 401 status code and handles freshing tokens.
 * Retries the original request once after refresh.
 *
 * @param fn - The downstream decorator function
 * @param req - a ServerRequest object
 * @returns a ServerResponse wrapped in a Promise
 */
const withTokenRefresh = (endpoint) => async (fn, req) => {
    var _a;
    const { refreshToken, accessToken } = req;
    const firstResponse = await fn({ ...req, token: accessToken });
    // Only deal with 401 response.
    if (((_a = firstResponse.response) === null || _a === void 0 ? void 0 : _a.status) !== 401)
        return firstResponse;
    // If we have no access code or refresh code, we're not logged in.
    if (!isToken(accessToken) || !isToken(refreshToken))
        return { ...firstResponse, error: errors.USER_LOGGED_OUT };
    // We have a refresh token, ask for a new access code.
    const refreshResponse = await post({ endpoint, token: refreshToken });
    const newAccess = refreshResponse.json && refreshResponse.json.accessToken;
    // If we still don't have an access code, we send back up an error.
    if (!isToken(newAccess))
        return { ...firstResponse, error: errors.USER_LOGGED_OUT };
    return await fn({ ...req, token: newAccess });
};
exports.withTokenRefresh = withTokenRefresh;
});

var withHandleAuth_1 = createCommonjsModule(function (module, exports) {
Object.defineProperty(exports, "__esModule", { value: true });
exports.withHandleAuth = void 0;

/*
 * Accepts a "server" parameter at configuration time.
 * Returns a decorator that attaches a server to a ServerRequest.
 *
 * @param server - a string representing a HTTP server
 * @returns a ServerDecorator function
 */
const withHandleAuth = (handler) => async (fn, req) => {
    const result = await fn(req);
    if (result.error === errors.USER_LOGGED_OUT)
        handler(result);
    return result;
};
exports.withHandleAuth = withHandleAuth;
});

var http = createCommonjsModule(function (module, exports) {
Object.defineProperty(exports, "__esModule", { value: true });
exports.configPost = exports.configGet = void 0;












const configGet = (c) => {
    const decFn = c.debug ? utilities.decorateDebug : utilities.decorate;
    return decFn(fetchBase_1.fetchBase, withGet_1.withGet, withCatch_1.withCatch, withServer_1.withServer(c.server), withHandleAuth_1.withHandleAuth(c.handleAuth || lodash.identity), withTokenRefresh_1.withTokenRefresh(c.endpointRefreshToken || ""), withURL_1.withURL, withStatus_1.withStatus, withJSON_1.withJSON);
};
exports.configGet = configGet;
const configPost = (c) => {
    const decFn = c.debug ? utilities.decorateDebug : utilities.decorate;
    return decFn(fetchBase_1.fetchBase, withPost_1.withPost, withCatch_1.withCatch, withServer_1.withServer(c.server), withHandleAuth_1.withHandleAuth(c.handleAuth || lodash.identity), withTokenRefresh_1.withTokenRefresh(c.endpointRefreshToken || ""), withURL_1.withURL, withStatus_1.withStatus, withJSON_1.withJSON);
};
exports.configPost = configPost;
});

var dist = createCommonjsModule(function (module, exports) {
Object.defineProperty(exports, "__esModule", { value: true });
exports.configPost = exports.configGet = void 0;

Object.defineProperty(exports, "configGet", { enumerable: true, get: function () { return http.configGet; } });
Object.defineProperty(exports, "configPost", { enumerable: true, get: function () { return http.configPost; } });
});

var configGet = dist.configGet;
var configPost = dist.configPost;
export { configGet, configPost };

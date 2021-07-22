/*
  Updates the local tenant.yaml file to match the current Auth0 configuration. This is necessary
  so that tenant.yaml matches any modifications made in the Auth0 UI, so that none of those modifications
  are overwritten on a new code push

  To be called via:
  yarn dump:[env]
*/

const { execSync } = require("child_process");
const fs = require("fs");
const YAML = require("yaml");
const _ = require("lodash");

const env = process.argv[2];

// Download up-to-date tenant.yaml config from Auth0
execSync(
  `a0deploy export --config_file ./config/${env}.json --strip --format yaml --output_folder .`,
  { stdio: "inherit" }
);

const {
  AUTH0_KEYWORD_REPLACE_MAPPINGS: mappings,
} = require(`../config/${env}.json`);
const stringMappings = {};
const arrayMappings = {};
// Generate mappings from ./config/[env].json
// Separated into string and array mappings
Object.keys(mappings).forEach((key) => {
  if (typeof mappings[key] === "string") {
    stringMappings[key] = mappings[key];
  } else if (Array.isArray(mappings[key])) {
    arrayMappings[key] = mappings[key];
  }
});

// Mappings of paths to environment variables that store their replacement
// This is done to remove social provider OAuth secrets that get pulled from Auth0
// The corresponding secrets should be stored in Github and are replaced at deploy-time
const censorMappings = {
  "connections[0].options.app_secret": "APPLE_OAUTH_SECRET",
  "connections[1].options.client_secret": "GOOGLE_OAUTH_SECRET",
};

const yaml = fs.readFileSync("./tenant.yaml", "utf8");
let config = YAML.parse(yaml);

for (key in censorMappings) {
  _.set(config, key, `##${censorMappings[key]}##`);
}

const listEqual = (l1, l2) => {
  let s = new Set(l2);
  return l1.length === l2.length && l1.every((v) => s.has(v));
};

// Traverse config object, replacing secrets with their keywords
const traverse = (obj) => {
  for (let k in obj) {
    if (!obj[k]) return;

    if (typeof obj[k] === "object") {
      if (Array.isArray(obj[k])) {
        // Check if equal to a list mapping
        for (let m in arrayMappings) {
          if (listEqual(obj[k], arrayMappings[m])) obj[k] = `@@${m}@@`;
        }
      }
      traverse(obj[k]);
    }

    if (typeof obj[k] === "string") {
      for (let s in stringMappings) {
        // Check if equal to a string mapping
        if (obj[k] === stringMappings[s]) obj[k] = `##${s}##`;
      }
    }
  }
};

traverse(config);

const newYaml = YAML.stringify(config, { schema: "yaml-1.1" });
// Remove strings from @@ keywords because Auth0 throws an error if we don't
const output = newYaml.replace(/\"@@|@@\"/g, "@@");

// Overwrite tentant.yaml with redacted variables
fs.writeFileSync("./tenant.yaml", output);

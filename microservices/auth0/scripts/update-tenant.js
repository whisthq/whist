const { execSync } = require('child_process')
const fs = require('fs')
const YAML = require('yaml')

execSync(
  'a0deploy export --config_file ./config/dev.json --strip --format yaml --output_folder .',
  {stdio: 'inherit'}
)

const env = process.argv[2]
const { AUTH0_KEYWORD_REPLACE_MAPPINGS: mappings } = require(`../config/${env}.json`);
const stringMappings = {}
const arrayMappings = {}
Object.keys(mappings).forEach((key) => {
  if(typeof mappings[key] === 'string') {
    stringMappings[key] = mappings[key]
  } else if(Array.isArray(mappings[key])) {
    arrayMappings[key] = mappings[key]
  }
});

const yaml = fs.readFileSync('./tenant.yaml', 'utf8')
let config = YAML.parse(yaml)

const listEqual = (l1, l2) => {
  let s = new Set(l2)
  return l1.length === l2.length && l1.every(v => s.has(v))
}

// Traverse config object, replacing config-specific values with their keywords
const traverse = (obj) => {
  for (let k in obj) {
    if(!obj[k]) return;

    if (typeof obj[k] === 'object') {
      if(Array.isArray(obj[k])) {
        // Check if equal to a list mapping
        for(let m in arrayMappings) {
          if(listEqual(obj[k], arrayMappings[m])) obj[k] = `@@${m}@@`
        }
      }
      traverse(obj[k])
    }

    if (typeof obj[k] === 'string') {
      for(let s in stringMappings) {
        if(obj[k] === stringMappings[s]) obj[k] = `##${s}##`
      }
    }
  }
}

traverse(config)

const newYaml = YAML.stringify(config, {schema: 'yaml-1.1'})
// Remove strings from @@ keywords because Auth0 throws an error if we don't
const output = newYaml.replace(/\"@@|@@\"/g, '@@')

fs.writeFileSync('./tenant.yaml', output)

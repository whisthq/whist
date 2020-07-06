import json, sys, simplejson

# Read command-line arguments

bucket = sys.argv[1]
version = sys.argv[2]

# Set package.json bucket

f = open("package.json")
raw_data = f.read()
f.close()

data = json.loads(raw_data)
data["build"]["publish"]["bucket"] = bucket
json_data = json.dumps(data)

f = open("package.json","w")
f.write(simplejson.dumps(simplejson.loads(json_data), indent=4, sort_keys=True))
f.close()

# Set app/package.json version

f = open("app/package.json")
raw_data = f.read()
f.close()

data = json.loads(raw_data)
data["version"] = version
json_data = json.dumps(data)

f = open("app/package.json","w")
f.write(simplejson.dumps(simplejson.loads(json_data), indent=4, sort_keys=True))
f.close()
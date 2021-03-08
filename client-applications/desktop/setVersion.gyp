import json, sys

# Read command-line arguments

bucket = sys.argv[1]
version = sys.argv[2]
notarize = sys.argv[3]

# Set package.json bucket

f = open("package.json")
raw_data = f.read()
f.close()

data = json.loads(raw_data)
data["build"]["publish"]["bucket"] = bucket

# on macOS, we need to notarize the client application before
# publishing, so that users can properly install it on their
# machines -- this is not needed if we're not publishing
if notarize == "false":
  data["build"]["afterSign"] = "build/moveLoadingandRename.js"
else:
  data["build"]["afterSign"] = "build/notarize.js"

json_data = json.dumps(data)

f = open("package.json","w")
f.write(json.dumps(json.loads(json_data), indent=4, sort_keys=True))
f.close()

# Set app/package.json version

f = open("app/package.json")
raw_data = f.read()
f.close()

data = json.loads(raw_data)
data["version"] = version
json_data = json.dumps(data)

f = open("app/package.json","w")
f.write(json.dumps(json.loads(json_data), indent=4, sort_keys=True))
f.close()

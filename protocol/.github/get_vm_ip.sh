#/bin/bash

# script to get vm ip from vm_name, web_server is optional
# usage: ./get_vm_ip.sh <vm_name> --web_server <web_server>

vm_name=$1

web_server=${web_server:-https://cube-celery-staging4.herokuapp.com/}

# get named params
while [ $# -gt 0 ]; do
   if [[ $1 == *"--"* ]]; then
        param="${1/--/}"
        declare $param="$2"
   fi
  shift
done

curl -s --location --request GET $web_server'/azure_vm/ip?vm_name='$vm_name
#| \ python3 -c "import sys, json; print(json.load(sys.stdin)['public_ip'])"

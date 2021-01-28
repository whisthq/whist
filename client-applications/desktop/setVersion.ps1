param (
    [string]$version = "",
    [string]$bucket = ""
)

# This script takes in a version number and a bucket name, and sets them 
# in the relevant package.json on Windows

$a = Get-Content 'app/package.json' -raw | ConvertFrom-Json
$a.version=([string]$version)
$a | ConvertTo-Json -depth 32| Set-Content 'app/package.json'

$a = Get-Content 'package.json' -raw | ConvertFrom-Json
$a.build.publish.bucket=([string]$bucket)
$a | ConvertTo-Json -depth 32| Set-Content 'package.json'

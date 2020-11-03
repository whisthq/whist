param (
    [string]$version = "",
    [string]$bucket = ""
)

$a = Get-Content 'app/package.json' -raw | ConvertFrom-Json
$a.version=([string]$version)
$a | ConvertTo-Json -depth 32| set-content 'app/package.json'

$a = Get-Content 'package.json' -raw | ConvertFrom-Json
$a.build.publish.bucket=([string]$bucket)
$a | ConvertTo-Json -depth 32| set-content 'package.json'
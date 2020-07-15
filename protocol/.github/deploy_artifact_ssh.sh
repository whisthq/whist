#!/bin/bash
set -e

# Deploy a folder directly to a VM, we would use this instead of the endpoint in CI because the github API
# does not provide access to artifacts until the run has completed.
# This script is fairly hard coded, but it only has the one use case at the moment.
# ./deploy_artifact_ssh.sh --ssh-key PATH --deploy PATH --vm-ip VAL

# Parse CLI args
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --ssh-key) ssh_key="$2"; shift ;;
        --vm-ip) vm_ip="$2"; shift ;;
        --vm-user) vm_user="$2"; shift ;;
        --deploy) path="$2"; shift ;;
        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
done

if [[ -z "$ssh_key" ]] || [[ -z "$path" ]] || [[ -z "$vm_ip" ]] || [[ -z "$vm_user" ]]; then
  echo "Missing required CLI argument (see script docs)"
fi

echo "SSH in and stop the service "
ssh -v -o StrictHostKeyChecking=no -i "$ssh_key" "$vm_user@$vm_ip" powershell.exe <<HEREDOC
net stop fractal ;
taskkill /IM "FractalService.exe" /F ;
taskkill /IM "FractalServer.exe" /F ;
Remove-Item C:\ProgramData\FractalCache\log.txt ;
HEREDOC

echo "scp across the build"
scp -v -o StrictHostKeyChecking=no -i "$ssh_key" -r "$path" "$vm_user@$vm_ip":/C:/server_build

echo "ssh in and restart the service and move the build folder"
ssh -v -o StrictHostKeyChecking=no -i sshkey "$vm_user@$vm_ip" powershell.exe <<HEREDOC
Copy-item -Force -Recurse C:\server_build\* -Destination 'C:\Program Files\Fractal' ;
Remove-Item C:\server_build\* ;
net start fractal ;
HEREDOC

#Copy-item -Force -Recurse build\* -Destination 'C:\Program Files\Fractal' ;
#Remove-Item C:\ProgramData\FractalCache\log.txt ;
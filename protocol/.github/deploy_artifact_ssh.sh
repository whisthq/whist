#/bin/bash

# Deploy a folder directly to a VM, we would use this instead of the endpoint in CI because the github API
# does not provide access to artifacts until the run has completed.
# This script is fairly hard coded, but it only has the one use case at the moment.
# ./deploy_artifact <ip> <path to deploy >

vm_ip=$1
path=$2

echo "SSH in and stop the service "
ssh -o StrictHostKeyChecking=no -i sshkey $SERVER_VM_USER@$SERVER_VM_IP powershell.exe " net stop fractal ;  taskkill /IM \"FractalService.exe\" /F;
  taskkill /IM \"FractalServer.exe\" /F; Remove-Item C:\ProgramData\FractalCache\log.txt ; "

echo "scp across the build"
scp -o StrictHostKeyChecking=no -i sshkey -r $path $SERVER_VM_USER@$SERVER_VM_IP:/C:/server_build

echo "ssh in and restart the service and move the build folder"
ssh -o StrictHostKeyChecking=no -i sshkey $SERVER_VM_USER@$SERVER_VM_IP powershell.exe " Copy-item -Force -Recurse C:\server_build\* -Destination 'C:\Program Files\Fractal';
 Remove-Item C:\server_build\* ;
  net start fractal ;"

#Copy-item -Force -Recurse build\* -Destination 'C:\Program Files\Fractal' ;
#Remove-Item C:\ProgramData\FractalCache\log.txt ;
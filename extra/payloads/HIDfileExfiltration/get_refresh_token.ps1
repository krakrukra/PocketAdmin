Write-Host "`nprovide your dropbox app key:"
$app_key = Read-Host

Write-Host "`nprovide your dropbox app secret:"
$app_secret = Read-Host

Write-Host "`nget access code manually by going to the following URL in web browser, confirm access"
Write-Host "https://www.dropbox.com/oauth2/authorize?client_id=${app_key}&response_type=code&token_access_type=offline"
Write-Host "`nprovide access code from webpage:"
$access_code = Read-Host

$body = New-Object "System.Collections.Generic.Dictionary[[String],[String]]"
$body.Add("client_id", "${app_key}")
$body.Add("client_secret", "${app_secret}")
$body.Add("grant_type", "authorization_code")
$body.Add("code", "${access_code}")
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
$response = Invoke-RestMethod -Uri "https://api.dropbox.com/oauth2/token" -Method Post -Body $body

Write-Host "`nrefresh token for your app:"
Write-Host $response.refresh_token
Read-Host

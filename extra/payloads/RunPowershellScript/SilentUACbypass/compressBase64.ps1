#turn command string into a byte array
Write-Host "type in your command to compress and then base64 encode:"
$Command = Read-Host
Write-Host "`n`n"
$Bytes = [System.Text.Encoding]::Unicode.GetBytes($Command)

#create memory stream object from byte array
$memStreamObj = New-Object System.IO.MemoryStream

#compress data in memory stream object with deflate method
$deflStreamObj = New-Object System.IO.Compression.DeflateStream ($memStreamObj, [System.IO.Compression.CompressionMode]::Compress)
$deflStreamObj.Write($Bytes, 0, $Bytes.Length)

#extract data from memory stream object into byte array
$deflStreamObj.Close()
$memStreamObj.Close()
$Bytes = $memStreamObj.ToArray()

#encode byte array into base64 string
$EncodedCommand =[Convert]::ToBase64String($Bytes)
Write-Host "base64 encoding:"
Write-Host "${EncodedCommand}"
Read-Host

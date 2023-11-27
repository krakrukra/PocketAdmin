#convert data from base64 encoded string into a byte array
Write-Host "type in your command to base64 decode and then decompress: "
$EncodedCommand = Read-Host
Write-Host "`n`n"
$Bytes = [System.Convert]::FromBase64String($EncodedCommand)

#create input memory stream object, fill it with byte array (it is compressed data)  
$memStreamObj1 = New-Object System.IO.MemoryStream
$memStreamObj1.Write($Bytes, 0, $Bytes.Length)
$memStreamObj1.Seek(0,0) | Out-Null

#create output memory stream object, fill it with decompressed data
$memStreamObj2 = New-Object System.IO.MemoryStream
$deflStreamObj = New-Object System.IO.Compression.DeflateStream($memStreamObj1, [System.IO.Compression.CompressionMode]::Decompress)
$deflStreamObj.CopyTo($memStreamObj2)
$deflStreamObj.Close()
$memStreamObj1.Close()

#read data from output memory stream, convert to unicode string
$Bytes = $memStreamObj2.ToArray()
$DecodedCommand = [System.Text.Encoding]::Unicode.GetString($Bytes)

#print out the results to the screen
Write-Host 'plaintext command:'
Write-Host "${DecodedCommand}"
Read-Host

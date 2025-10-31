This is a windows specific payload, which will install a  
new SSL root certificate into the target machine. This  
can be used to configure a bunch of clients to use your  
web server via secure connection, without having to buy  
a certificate from an existing authority. Alternatively,  
this can be used for penetration testing purposes, in  
case you want your web server to intercept HTTPS traffic  
that was originally intended for another server.  
  
After insertion, device will execute commands from **payload.txt**  
file. This will bring up windows runline dialog and use it to find  
the correct USB drive based on FAT volume label (POCKETADMIN) and  
then start a powershell script contained in **insert.ps1** file.  
It will be launched with request for administrative privileges,  
so currently logged in user has to have admin rights for script  
to work properly.  
  
Certificate to be installed on a target machine is located in  
**ca-certificate.pem** file inside the payload directory. You  
**must** replace the default file provided here with your own.  
To generate new SSL certificates you can use openssl utility.  
For convenience, some bash scripts are provided inside **rootCA**  
directory to automate the process. You should first run commands  
in **generateRootCA.sh** to create a new CA certificate and encrypted  
private key. Then, you can proceed to generating certificates for  
your web server. To do that, run **generateCertChain.sh** script.  
New web server certificate and private key will be created and moved  
to **website/ssl** directory.  
  
Besides adding a new SSL root certificate authority, **insert.ps1**  
script will also add new DNS entries into the windows **hosts** file,  
so make sure to modify default values used in **insert.ps1** with IP's  
and domain names of your specific web server. These DNS names must match  
the subjectAltName value specified in your website's SSL certificate.  
  
Note that the contents of **rootCA** and **website** directories are  
not meant to be copied to the PocketAdmin's USB drive. CA private  
key should be kept safe from any unauthorized access, and website  
directory contains configuration files for an example web server.  
In this particular example payload, web server is expected to be  
an apache2 software (httpd), installed on a Debian Linux system.  
If that is the case, you should be able to simply copy website  
directory from the payload into **/var/www/** directory on your  
Debian VPS, and then configure the web server by running a bash  
script located at **/var/www/website/config/configureApache2.sh**  
  
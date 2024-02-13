This is a windows specific payload, which will work in HID-only mode.  
It can be used in situations where normal operation of built-in USB  
disk is not possible, eg due to security policies of whitelisting or  
complete blocking of USB storage devices. This payload will upload  
files from the home directory of a currently logged in user to your  
dropbox account. It works by downloading a powershell script called  
upload.ps1 in the repository into the target machine and then running  
it. This script has to be modified and then actually posted on a web  
server where anyone can read it (eg on this same dropbox account).  
  
Before you can use the payload you have to make an account with  
dropbox and create a new dropbox app. All of this can be done  
through their website. You will also have to go to permissions tab  
for your app and enable any files.metadata and files.content access.  
  
You **must** then replace APP_KEY_HERE, APP_SECRET_HERE, REFRESH_TOKEN_HERE  
strings with correct values inside your upload.ps1 file in order to  
receive anything. Values of app_key and app_secret are available via  
dropbox webpage dedicated to your app. To get refresh token you will have  
to run interactive powershell script called get_refresh_token.ps1 and  
follow the instructions it provides. By default, hidden files and files  
larger than 200MiB are not transferred to save time and space on your  
dropbox storage, you can remove these filters from upload.ps1 if needed.  
  
After your script was modified so it actually connects to your specific  
dropbox app you will need to post it online. You can do it by uploading  
upload.ps1 file to your dropbox. Then navigate to this file, select it,  
click "Share" -> "Create and copy link" and paste this link into  
payload.txt file (instead of URL_OF_YOUR_SCRIPT_HERE string). You will  
have to also change &dl=0 parameter inside URL to &dl=1 to make it work.  
  
After all this was done, you can copy your payload files to PocketAdmin's  
storage. You only need payload.txt and config.txt, but can copy them all.  
From that point on payload should run successfully, but keep in mind that  
transferring large quantities of data can take some time.  
  
This is a windows specific payload, which will gather various  
information about a target machine and it's user in a  
non-invasive way. That is, without installing or launching any  
other software that was not included in the target system.  
Collected data can be useful by itself or can later be used for  
developing a more specialized payload for this same machine.  
  
After insertion, device will execute commands from **payload.txt**  
file. This will bring up windows runline dialog and use it to find  
the correct USB drive based on FAT volume label (POCKETADMIN) and  
then start a powershell script contained in **insert.ps1** file.  
It will be launched with request for administrative privileges,  
so currently logged in user has to have admin rights for script  
to work properly.  
  
All the data collected will be saved in a zip file on POCKETADMIN's  
USB storage with a name in this format: YYYYMMDD_hhmmss.zip, where YYYY  
is the year, MM - month, DD - day of month, hh - hour of the day in  
24-hour format, mm - minute, ss - second of when insert.ps1 was launched.  
So, every time you plug the device in, a new zip archive will be created.  
Information collected includes web browser data (from firefox, chrome  
and edge as these are very popular), SAM and SYSTEM registry hives  
(which allows to extract password hashes for local user accounts), saved  
Wi-Fi profiles with passwords in cleartext, network configuration, names  
of local user accounts and groups, lists of installed applications and  
scheduled tasks, powershell command history for current user and list of  
files in his home folder, clipboard contents at the time of insertion  
(if it was text), generic information about the target computer and OS,  
lists of active processes and services, etc.  
  
Browser data includes cookies, bookmarks, download and browsing history,  
web form autofill data, browser-stored passwords, and some other things.  
For the most part it comes in a form of SQLite and JSON files. JSON can  
be viewed in a text editor or a web browser, while SQLite files can be  
read by specialized SQL database softwares (eg [sqlitebrowser](https://sqlitebrowser.org)). Some of  
the data inside those database files is encrypted, so you will have to  
make additional steps to get the data out and use it. In order to decrypt  
saved passwords you can use python scripts provided in payload directory,  
these are [firefox_decrypt.py](https://github.com/unode/firefox_decrypt) and chrome_decrypt_v10.py;  
These scripts are not necessary to extract the data from target machine,  
so you may not copy them to PocketAdmin's USB storage, but use them later  
to decrypt data on a different computer. Here are the example commands for  
how you can do that, assuming you unpacked the YYYYMMDD_hhmmss.zip file in  
the same directory where python scripts are located:  
  
> python3 firefox_decrypt.py YYYYMMDD_hhmmss/firefox/something.default-esr  
> python3 chrome_decrypt_v10.py -d YYYYMMDD_hhmmss/chrome  
> python3 chrome_decrypt_v10.py -d YYYYMMDD_hhmmss/edge  
  
Make sure to replace "YYYYMMDD_hhmmss" with your actual directory name and  
"something.default-esr" with the name of your actual firefox profile. As  
you can see chrome_decrypt_v10.py script works for both chrome and edge web  
browsers. For older versions of edge and chrome you may use the same AES256  
key saved in decrypted_key_v10.bin to decrypt cookie values as well. But  
be aware that since chrome version 127 cookies are encrypted with v20 method.  
  
SAM and SYSTEM files collected on a target machine can be used to extract  
LM and NT hashes for local user account passwords. For example, it can be  
done using a python script pwdump.py from a packege called [creddump7](https://github.com/CiscoCXSecurity/creddump7);  
Those hashes may be used directly in some circumstances or be used for  
cracking the original local user passwords. For example, if you choose to  
crack passwords using [hashcat](https://hashcat.net/hashcat) utility you could run commands like this  
(assuming you are on a debian PC and have creddump7 already installed):  
  
> /usr/share/creddump7/pwdump.py SYSTEM SAM > hashlist.txt  
> hashcat -a 0 -m 1000 hashlist.txt wordlist.txt  
  
This sequence of commands will use pwdump.py script to extract LM and NT  
hashes from SYSTEM and SAM files and save the results inside hashlist.txt;  
Then hashcat will hash every line inside wordlist.txt and then check if  
those hashes match the ones you extracted. If a match is found, you have  
successfully found the right password. There are many ways of how you can  
get or generate a wordlist, as well as many other possible modes of  
operation for hashcat, so feel free to do more research on that if needed.  
  
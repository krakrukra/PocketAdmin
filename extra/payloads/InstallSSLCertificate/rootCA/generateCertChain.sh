#!/bin/bash

printf "\ngenerating website private key... "
openssl genrsa -out website-privateKey.pem 4096
printf "done\n"

printf "\ngenerating website certificate signing request...\n"
openssl req -new -sha256 -key website-privateKey.pem -out website-signRequest.pem

printf "\nspecify subjectAltName value, eg subjectAltName=DNS:website.here,DNS:*.website.here,IP:192.168.0.102"
printf "\nsubjectAltName="
read subjectAltName
if [ "$subjectAltName" = "" ];then
   subjectAltName="DNS:website.here,DNS:*.website.here,IP:192.168.0.102"
fi
printf "subjectAltName=%s" "$subjectAltName" > "CertChain_extentions.conf"

printf "\nCA generating website certificate from a request...\n"
openssl x509 -req -sha256 -days 3650 -in website-signRequest.pem -CA ca-certificate.pem -CAkey ca-privateKey.pem -out website-certificate.pem -extfile CertChain_extentions.conf -CAcreateserial
openssl x509 -in website-certificate.pem -text

printf "\ngenerating full chain of certificates... "
cat website-certificate.pem > website-fullChain.pem
cat ca-certificate.pem >> website-fullChain.pem
printf "done\n"

printf "\nmoving .pem files into website/ssl directory... "
mv website-privateKey.pem ../website/ssl/
mv website-signRequest.pem ../website/ssl/
mv website-certificate.pem ../website/ssl/
mv website-fullChain.pem ../website/ssl/
printf "done\n"

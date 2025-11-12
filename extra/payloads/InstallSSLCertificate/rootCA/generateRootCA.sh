#!/bin/bash
set -e

printf "\ngenerating CA encrypted private key...\n"
openssl genrsa -aes256 -out ca-privateKey.pem 4096

printf "\ngenerating CA certificate...\n"
openssl req -new -x509 -sha256 -days 3650 -key ca-privateKey.pem -out ca-certificate.pem
openssl x509 -in ca-certificate.pem -text

printf "\ncopying CA certificate file into payload directory... "
cp ca-certificate.pem ../
printf "done\n"

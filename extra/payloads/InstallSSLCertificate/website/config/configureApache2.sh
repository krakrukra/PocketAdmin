#!/bin/bash

sudo cp apache2.conf /etc/apache2/
sudo cp website.conf /etc/apache2/sites-available/
sudo cp website-ssl.conf /etc/apache2/sites-available/
sudo a2enmod ssl
sudo a2ensite website-ssl.conf
sudo systemctl restart apache2.service

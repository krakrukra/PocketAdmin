#!/usr/bin/env python3

import os
import sqlite3
import shutil
from Cryptodome.Cipher import AES
import argparse

def decrypt_value(buff, master_key):
  try:
    iv = buff[3:15]
    payload = buff[15:]
    cipher = AES.new(master_key, AES.MODE_GCM, iv)
    return cipher.decrypt(payload)[:-16].decode()
  except:
    return "decryption failed"

def display_credentials(url, username, decrypted_password):
  separator = "-" * 60
  print(separator)
  print(f"URL: {url}")
  print(f"User Name: {username}")
  print(f"Password: {decrypted_password}")
  print(separator)
  print("\n")

if __name__ == '__main__':
  parser = argparse.ArgumentParser(description="Retrieve browser credentials.")
  parser.add_argument("-d", "--dir", help="Path to the browser data directory.", required=True)
  
  args = parser.parse_args()
  os.chdir(args.dir)
  
  with open('decrypted_key_v10.bin', 'rb') as f:
    master_key = f.read()
  
  with sqlite3.connect('Login Data.sqlite') as conn:
    cursor = conn.cursor()

  for row in cursor.execute("SELECT action_url, origin_url, username_value, password_value FROM logins"):
    action_url = row[0]
    origin_url = row[1]
    url = action_url if action_url else origin_url
    username = row[2]
    encrypted_password = row[3]
    decrypted_password = decrypt_value(encrypted_password, master_key)
    display_credentials(url, username, decrypted_password)
    

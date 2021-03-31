#!/bin/bash
trap "kill 0; ssh somename@linodeVPS -p 2200 'powershell Stop-Process -Name ffmpeg" EXIT

filename=$(date +%y%m%d_%H%M%S.ts)
ffmpeg -y -f rtsp -rtsp_transport tcp -rtsp_flags listen -i rtsp://127.0.0.1:5554 -c copy -f mpegts ./$filename -c copy -f mpegts udp://127.0.0.1:5554 &
ssh -R 127.0.0.1:5554:127.0.0.1:5554 somename@linodeVPS -p 2200 'powershell -exec bypass "C:\ProgramData\stream\sendstream.ps1"' &
ffplay udp://127.0.0.1:5554

wait

#!/bin/bash

tmux new-session -d -s lab2
tmux send-keys 'cd ~/cs252/lab2-src' 'C-m'
tmux send-keys './script-consumer.sh' 'C-m'
tmux rename-window 'Monitor'
tmux select-window -t lab2:0
tmux split-window -h
tmux send-keys 'htop -u $USER' 'C-m'
tmux split-window -v -t 0
sleep 1
tmux send-keys 'cd ~/cs252/lab2-src' 'C-m'
tmux send-keys './monitor.sh $(ps -u $USER | grep script-consumer | awk "{print $1}") -cpu 20 mem 2000000 4 2' 'C-m'
tmux split-window -v -t 1
tmux send-keys 'cd ~/cs252/lab2-src' 'C-m'
tmux send-keys "watch 'bash -c \"cat reports_dir/* \" '" 'C-m'

tmux attach-session -t lab2

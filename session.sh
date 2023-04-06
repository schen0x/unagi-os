#!/bin/bash
# Use in subfolder, kick-off the standard workspace.
# ln -s <thisfile> ./u-<subfolder>/session.sh

session="os"

tmux new-session -d -s $session &&

{
# Run on first time only 

window=0
tmux rename-window -t $session:$window '-'
tmux send-keys -t $session:$window "cd ../; nvim README-os.md;"

window=1
tmux new-window -t $session:$window -n 's'

window=2
tmux new-window -t $session:$window -n 'r'

window=3
tmux new-window -t $session:$window -n 'g'

window=4
tmux new-window -t $session:$window -n 'n'

# Select the first window
tmux select-window -t $session:0

};

tmux attach-session -t $session

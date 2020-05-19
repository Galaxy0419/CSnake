#!/bin/bash
rm -rf ./.git
git init
git add .
git commit -a -m 'Initial Commit'
git remote add origin git@github.com:Galaxy0419/CSnake.git
git push -f origin master
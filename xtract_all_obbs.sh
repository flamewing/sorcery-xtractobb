#!/bin/bash
# Delete everything
mkdir -p sorcery{1,2,3}{obb,json} sorcery{1,2,3}json/FightScenes
find sorcery{1,2,3}{obb,json} -type f -delete

# Extract Sorcery! 1 files
./xtractobb com.inkle.sorcery1/main.1200.com.inkle.sorcery1.obb sorcery1obb/
cd sorcery1obb; find . -iname '*.json' | while read ff; do cp $ff ../sorcery1json/$ff; done; cd ..
cd sorcery1obb; find . -iname '*.inkcontent' | while read ff; do cp $ff ../sorcery1json/$ff; done; cd ..

# Extract Sorcery! 2 files
./xtractobb com.inkle.sorcery2/main.1100.com.inkle.sorcery2.obb sorcery2obb/
cd sorcery2obb; find . -iname '*.json' | while read ff; do cp $ff ../sorcery2json/$ff; done; cd ..
cd sorcery2obb; find . -iname '*.inkcontent' | while read ff; do cp $ff ../sorcery2json/$ff; done; cd ..

# Extract Sorcery! 3 files
./xtractobb com.inkle.sorcery3/main.1030.com.inkle.sorcery3.obb sorcery3obb/
cd sorcery3obb; find . -iname '*.json' | while read ff; do cp $ff ../sorcery3json/$ff; done; cd ..
cd sorcery3obb; find . -iname '*.inkcontent' | while read ff; do cp $ff ../sorcery3json/$ff; done; cd ..


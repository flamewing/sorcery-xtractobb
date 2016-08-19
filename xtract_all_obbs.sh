#!/bin/bash
# Delete everything
find sorcery{1,2,3}{obb,json} -type f -delete

# Extract Sorcery! 1 files
./xtractobb com.inkle.sorcery1/main.200.com.inkle.sorcery1.obb sorcery1obb/
./xtractobb com.inkle.sorcery1/patch.203.com.inkle.sorcery1.obb sorcery1obb/
cd sorcery1obb; find . -iname '*.json' | while read ff; do cp $ff ../sorcery1json/$ff; done; cd ..
cd sorcery1obb; find . -iname '*.inkcontent' | while read ff; do cp $ff ../sorcery1json/$ff; done; cd ..

# Extract Sorcery! 2 files
./xtractobb com.inkle.sorcery2/main.100.com.inkle.sorcery2.obb sorcery2obb/
./xtractobb com.inkle.sorcery2/patch.102.com.inkle.sorcery2.obb sorcery2obb/
cd sorcery2obb; find . -iname '*.json' | while read ff; do cp $ff ../sorcery2json/$ff; done; cd ..
cd sorcery2obb; find . -iname '*.inkcontent' | while read ff; do cp $ff ../sorcery2json/$ff; done; cd ..

# Extract Sorcery! 3 files
./xtractobb com.inkle.sorcery3/assets/sorcery.ogg sorcery3obb/
cd sorcery3obb; find . -iname '*.json' | while read ff; do cp $ff ../sorcery3json/$ff; done; cd ..
cd sorcery3obb; find . -iname '*.inkcontent' | while read ff; do cp $ff ../sorcery3json/$ff; done; cd ..


#!/bin/bash
# Delete everything
rm -rf sorcery{1,2,3}{obb,json}

# Extract Sorcery! 1 files
./xtractobb com.inkle.sorcery1/main.1200.com.inkle.sorcery1.obb sorcery1obb/

# Extract Sorcery! 2 files
./xtractobb com.inkle.sorcery2/main.1100.com.inkle.sorcery2.obb sorcery2obb/

# Extract Sorcery! 3 files
./xtractobb com.inkle.sorcery3/main.1030.com.inkle.sorcery3.obb sorcery3obb/

mkdir -p sorcery{1,2,3}json/FightScenes
for linkdir in 1 2 3; do
	find sorcery${linkdir}obb -iname '*.json' -or -iname '*.inkcontent' | while read ff; do
		link=sorcery${linkdir}json/${ff#sorcery${linkdir}obb/}
		ln -fns ../$ff $link
	done
done


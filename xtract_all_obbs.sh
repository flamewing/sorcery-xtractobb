#!/bin/bash
# Delete everything
rm -rf output/sorcery{1,2,3,4}{obb,json}

# Extract Sorcery! 1 files
./xtractobb com.inkle.sorcery1/main.14002.com.inkle.sorcery1.obb output/sorcery1obb/

# Extract Sorcery! 2 files
./xtractobb com.inkle.sorcery2/main.13002.com.inkle.sorcery2.obb output/sorcery2obb/

# Extract Sorcery! 3 files
./xtractobb com.inkle.sorcery3/main.12002.com.inkle.sorcery3.obb output/sorcery3obb/

# Extract Sorcery! 4 files
./xtractobb com.inkle.sorcery4/main.11002.com.inkle.sorcery4.obb output/sorcery4obb/

pushd output || return
mkdir -p sorcery{1,2,3,4}json/FightScenes

for linkdir in 1 2 3 4; do
	find sorcery${linkdir}obb -iname '*.json' -or -iname '*.inkcontent' | while read -r ff; do
		link=sorcery${linkdir}json/${ff#sorcery${linkdir}obb/}
		ln -fns "$(pwd)/$ff" "$link"
	done
done

popd || return

#!/usr/bin/env python3
# Quick script to add program hashes to the CHIP-8 database

import json
import zlib

newData = {}

with open("../programs.json") as f:
    data = json.load(f)

    for game in data:
        if 'crc32' not in data[game]:
            with open(f"roms/{game}.ch8", "rb") as g:
                hash = zlib.crc32(g.read())
                print(f"{game} : {hex(hash)}")
                data[game]['crc32'] = f"{hash:x}"
        
        hash = data[game]['crc32']

        newData[hash] = data[game]
        del newData[hash]['crc32']
    
    with open("programs_new.json", "w") as output:
        json.dump(newData,output,indent=4)

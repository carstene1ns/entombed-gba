E:\devkitPro\devkitARM\bin\mmutil.exe ankh.wav arrow.wav arrow_hit.wav bow.wav bullet.wav change.wav checkpoint.wav climb.wav coin.wav door.wav enemy_die.wav exit.wav jump.wav key.wav player_die.wav teleport.wav title.wav urn_break.wav walk.wav -osoundbank.bin -hsoundbank.h
E:\devkitPro\devkitARM\bin\bin2s soundbank.bin > soundbank.s
rem Move all files to their correct folder
del *.bin
move *.s ../../src
move *.h ../../include
rem *Background palette* from tilemap_walls_l1 - Black is transparent
E:\devkitPro\devkitARM\bin\grit.exe tilemap_walls_l1.bmp -p -pT0 -pu16 -pn32 -g! -ftc -opal_bg -spal_bg

rem *Sprite palette* from spr_player - Black is transparent
E:\devkitPro\devkitARM\bin\grit.exe spr_player.bmp -p -pu16 -pn16 -g! -ftc -opal_oam -spal_oam

rem *Title screen sprites* (32x32) u16
E:\devkitPro\devkitARM\bin\grit.exe spr_title.bmp -p! -gt -gu16 -gB4 -Mw4 -Mh4 -ftc -ospr_title -sspr_title

rem *Level selector sprites* (32x32) u16
E:\devkitPro\devkitARM\bin\grit.exe spr_levelselect.bmp -p! -gt -gu16 -gB4 -Mw4 -Mh4 -ftc -ospr_levelselect -sspr_levelselect

rem *Fixtures* (32x16) u16
E:\devkitPro\devkitARM\bin\grit.exe tilemap_fixtures.bmp -p! -gt -gu16 -gB4 -Mw4 -Mh2 -ftc -otilemap_fixtures -stilemap_fixtures

rem *Walls" (32x16) u16
E:\devkitPro\devkitARM\bin\grit.exe tilemap_walls.bmp -p! -gt -gu16 -gB4 -Mw4 -Mh2 -ftc -otilemap_walls -stilemap_walls

rem *Player Sprite* (32x32) u16
E:\devkitPro\devkitARM\bin\grit.exe spr_player.bmp -p! -gt -gu16 -gB4 -Mw4 -Mh4 -ftc -ospr_player -sspr_player

rem *Projectiles sprites* (8x8) u16
E:\devkitPro\devkitARM\bin\grit.exe spr_projectiles.bmp -p! -gt -gu16 -gB4 -Mw1 -Mh1 -ftc -ospr_projectiles -sspr_projectiles

rem *Enemies Sprites* (32x32) u16
E:\devkitPro\devkitARM\bin\grit.exe spr_enemies.bmp -p! -gt -gu16 -gB4 -Mw4 -Mh4 -ftc -ospr_enemies -sspr_enemies

rem *Moving platform Sprite* (32x16) u16
E:\devkitPro\devkitARM\bin\grit.exe spr_platform.bmp -p! -gt -gu16 -gB4 -Mw4 -Mh2 -ftc -ospr_platform -sspr_platform

rem *Breaking urn Sprite* (32x32) u16
E:\devkitPro\devkitARM\bin\grit.exe spr_urn.bmp -p! -gt -gu16 -gB4 -Mw4 -Mh4 -ftc -ospr_urn -sspr_urn

rem *Game over Sprite* (32x16) u16
E:\devkitPro\devkitARM\bin\grit.exe spr_gameover.bmp -p! -gt -gu16 -gB4 -Mw4 -Mh2 -ftc -ospr_gameover -sspr_gameover

rem *Coin scores Sprites* (32x8) u16
E:\devkitPro\devkitARM\bin\grit.exe spr_coinscores.bmp -p! -gt -gu16 -gB4 -Mw4 -Mh1 -ftc -ospr_coinscores -sspr_coinscores

rem *Hourglass digit Sprites* (16x16) u16
E:\devkitPro\devkitARM\bin\grit.exe spr_digits.bmp -p! -gt -gu16 -gB4 -Mw2 -Mh2 -ftc -ospr_digits -sspr_digits

rem *Moving blocks Sprites* (32x16) u16
E:\devkitPro\devkitARM\bin\grit.exe spr_blocks.bmp -p! -gt -gu16 -gB4 -Mw4 -Mh2 -ftc -ospr_blocks -sspr_blocks

rem *Font* (8x8) - Has own palette
E:\devkitPro\devkitARM\bin\grit.exe entombed_font.bmp -p -pn16 -gt -gu16 -gB4 -Mw1 -Mh1 -ftc -oentombed_font -sentombed_font

rem Move all files to their correct folder
move *.c ../../src/gfx
move *.h ../../include/gfx


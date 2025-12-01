
#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <gba_dma.h>
#include <gba_sprites.h>
#include <stdio.h>
#include <stdlib.h>
// Include user files here
#include "gbaBGTiles_tile_pal.h"
#include "gbaSpriteSheet_tile_pal.h"
// End user file includes


// Add user macros here

// End user macros


// Include extern prototypes here

	extern const unsigned char LegendOfLinkBG_tilemap[];
	extern const unsigned int LegendOfLinkBG_len;

	extern const unsigned char LegendOfLinkFG_tilemap[];
	extern const unsigned int LegendOfLinkFG_len;

	extern const u16 gbaSpriteSheet_pal[];
	extern const u8 gbaSpriteSheet_tiles[];

// End extern here


//Set Variables here

const int bg0_base = 8;
const int bg1_base = 16;

//End Variable here

// Set settings for backgrounds 1 and 0
inline void setBGSettings()
{ 
	//Share the same tile set
	//Change priorities so bg 1 is in front of bg 0
	REG_BG0CNT = 
	BG_256_COLOR 			|	//Use 256-color palette
	BG_TILE_BASE(0) 		|	//Set the starting address for each tilemap
	TEXTBG_SIZE_512x512 	|	//Set bg to 512x512 pixels
	BG_PRIORITY(1) 			|
	BG_MAP_BASE(bg0_base);

	REG_BG1CNT = 
	BG_256_COLOR 			|	//Use 256-color palette
	BG_TILE_BASE(0) 		|	//Set the starting address for each tilemap
	TEXTBG_SIZE_512x512 	|	//Set bg to 512x512 pixels
	BG_PRIORITY(0) 			|	
	BG_MAP_BASE(bg1_base);		
	
	return;
}

// Load 256-color palette for bg
static inline void loadBGPalette(const u16 *palette)
{
dmaCopy(palette, BG_PALETTE, 256); //number_of_16_bit_words = 256
}
// Load set of tiles for bg
static inline void loadBGTileSet(const u8 *tileset)
{
dmaCopy(tileset, TILE_BASE_ADR(0), 16384);
}

// Load background tile map
static inline void loadGroundTileMap()
{
CpuFastSet(LegendOfLinkBG_tilemap, MAP_BASE_ADR(bg0_base), LegendOfLinkBG_len);
}

// Load foreground/collidable tile map
static inline void loadForegroundMap()
{
CpuFastSet(LegendOfLinkFG_tilemap, MAP_BASE_ADR(bg1_base), LegendOfLinkFG_len); //Upper-left quadrant
}

// Load sprite palette
static inline void loadSpritePalette(const u16 *palette)
{
	dmaCopy(palette, SPRITE_PALETTE, 256); //number_of_16_bit_words = 256
}

// Load sprite tiles
static inline void loadSpriteTileSet(const u8 *tileset)
{
dmaCopy(tileset, SPRITE_GFX, 16384);
}

inline void setVideoSettings()
{
	//Set video settings (find in cowbite or gba_video.h)
	REG_DISPCNT =
	MODE_0 		| // Enable mode 0 (regular tiled backgrounds)
	OBJ_ENABLE 	| // Enable sprites (often referred to as "objects")
	OBJ_1D_MAP 	| // Set sprites to be drawn with sequentially stored tiles
	BG0_ENABLE 	| // Enable background 0
	BG1_ENABLE	; // Enable background 1
	return;
}

//---------------------------------------------------------------------------------
// Program entry point
//---------------------------------------------------------------------------------
int main(void) 
{
//---------------------------------------------------------------------------------


	// the vblank interrupt must be enabled for VBlankIntrWait() to work
	// since the default dispatcher handles the bios flags no vblank handler
	// is required; do not remove
	irqInit();
	irqEnable(IRQ_VBLANK);

	///// User add code here /////

	//Set settings here
	setVideoSettings();
	setBGSettings();
	//Initialize variables here

	loadBGPalette(gbaBGTiles_pal);
	VBlankIntrWait();
	

	loadBGTileSet(gbaBGTiles_tiles);
	VBlankIntrWait();

	// Load sprite palette
	loadSpritePalette(gbaSpriteSheet_pal);
	VBlankIntrWait();

	// Load sprite palette
	loadSpriteTileSet(gbaSpriteSheet_tiles);
	VBlankIntrWait();

	//Load background and foreground maps
	loadGroundTileMap();
	VBlankIntrWait();
	// Load foreground/collidable tile map
	loadForegroundMap();
	VBlankIntrWait();

	

	while (1) 
	{
		VBlankIntrWait(); // Wait until the next frame starts; Do not remove
	}



}



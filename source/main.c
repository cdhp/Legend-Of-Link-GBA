
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
// End user file includes


// Add user macros here

// End user macros


// Include extern prototypes here

	extern const unsigned char LegendOfLinkBG_tilemap[];
	extern const unsigned int LegendOfLinkBG_len;

	extern const unsigned char LegendOfLinkFG_tilemap[];
	extern const unsigned int LegendOfLinkFG_len;

// End extern here


//Set Variables here

const int bg0_base = 8;
const int bg1_base = 16;

//End Variable here

// Load 256-color palette for bg
static inline void loadBGPalette(const u16 *palette){
dmaCopy(palette, BG_PALETTE, 256); //number_of_16_bit_words = 256
}
// Load set of tiles for bg
static inline void loadBGTileSet(const u8 *tileset){
dmaCopy(tileset, TILE_BASE_ADR(0), 16384);
}

// Load background tile map
static inline void loadGroundTileMap()
{
CpuFastSet(LegendOfLinkBG_tilemap, MAP_BASE_ADR(bg1_base), LegendOfLinkBG_len);
}
// Load foreground/collidable tile map
static inline void loadForegroundMap()
{
CpuFastSet(LegendOfLinkFG_tilemap, MAP_BASE_ADR(bg0_base), LegendOfLinkFG_len); //Upper-left quadrant
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


// Set settings for backgrounds 1 and 0
inline void setBGSettings()
{ 
	//Share the same tile set
	//Change priorities so bg 1 is in front of bg 0
	REG_BG0CNT = BG_256_COLOR | BG_TILE_BASE(0) | TEXTBG_SIZE_512x512 |
	BG_PRIORITY(0) | BG_MAP_BASE(bg0_base);
	REG_BG1CNT = BG_256_COLOR | BG_TILE_BASE(0) | TEXTBG_SIZE_512x512 |
	BG_PRIORITY(1) | BG_MAP_BASE(bg1_base);
	//Use 256-color palette //Set bg to 512x512 pixels 
	//Set the starting address for each tilemap
	return;
}

//---------------------------------------------------------------------------------
// Program entry point
//---------------------------------------------------------------------------------
int main(void) {
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

	//Load background and foreground maps
	loadGroundTileMap();
	// Load foreground/collidable tile map
	loadForegroundMap();

	while (1) 
	{
		VBlankIntrWait(); // Wait until the next frame starts; Do not remove
	}



}



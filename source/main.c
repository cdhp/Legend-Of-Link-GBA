
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
#define SPRT ((volatile Sprite*) 0x07000000)
// End user macros


// Include extern prototypes here

	extern const unsigned char LegendOfLinkBG_tilemap[];
	extern const unsigned int LegendOfLinkBG_len;

	extern const unsigned char LegendOfLinkFG_tilemap[];
	extern const unsigned int LegendOfLinkFG_len;

	extern const u16 gbaSpriteSheet_pal[];
	extern const u8 gbaSpriteSheet_tiles[];

// End extern here


// Bomerang
typedef struct
{
    bool active;
    s16 x, y;        	// current position
    s16 vx, vy;      	// velocity
    u16 sprnum;      	// sprite index
    u16 frame;
	u16 frameTimer;
} Boomerang;

//Set Variables here

const int bg0_base = 8;
const int bg1_base = 16;

Boomerang booms[10];
bool prevB; 			// B-pressed value from the previous frame
int scrollPosX;
int scrollPosY;

//End Variable here

// init variables

void initVars()
{
	prevB = false;
	scrollPosX = 0;
	scrollPosY = 0;
}

// end init variables

// LINK

typedef enum
{
	DIR_UP = 0,
	DIR_RIGHT,
	DIR_DOWN,
	DIR_LEFT		
}Direction;

struct Link
{
	u16 px;
	u16 py;
	u16 x;
	u16 y;
	bool isMoving;
	short topSpeed;
	Direction dir;
	u16 sprnum;
	u16 frame;
	u16 frameTimer;
};
struct Link link = 
{
	.px = 50,
	.py = 50,
	.x = 50,
	.y = 50,
	.isMoving = false,
	.topSpeed = 1,
	.dir = DIR_DOWN,
	.sprnum = 0,
	.frame = 0,
	.frameTimer = 0
};



// End Link

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

// Sprite setup
static inline void setupLinkSprite()
{
	SPRT[link.sprnum].ColorMode = 1; 			//Set it to 256 color mode
	SPRT[link.sprnum].Priority = 1; 			//Make him below foreground level
	SPRT[link.sprnum].Shape = SQUARE; 			//All link graphics are 16x16 pixel squares
	SPRT[link.sprnum].Size = Sprite_16x16; 		//16x16 pixel squares
	SPRT[link.sprnum].Character = 4<<1; 		//Tiles are "twice as large" in 256-color mode
	SPRT[link.sprnum].X = 50;
	SPRT[link.sprnum].Y = 50;
}

void setupBoomerangSprites()
{
	// max of 10  boomerangs
    for(int i = 1; i <= 10; i++)
    {
        SPRT[i].ColorMode = 1;
        SPRT[i].Priority = 1;
        SPRT[i].Shape = SQUARE;
        SPRT[i].Size = Sprite_16x16;
        SPRT[i].Character = 132<<1;   
        SPRT[i].X = 240;  // hide offscreen
        SPRT[i].Y = 160;
    }

    for(int i = 0; i < 10; i++)
    {
        booms[i].active = false;
        booms[i].sprnum = i + 1;  // sprite slot 1–10 (Link uses 0)
    }
}

// end Sprite setup


u16 getForegroundTile(u16 x, u16 y)
{
	int xtile = x>>3;
	int ytile = y>>3;
	if (xtile < 32)
	{
		if(ytile < 32) //upper left quadrant (0)
		{
			//y * 32
			return ((vu16*) MAP_BASE_ADR(bg1_base))[(ytile<<5) + xtile];
		}
		else //lower left quadrant (2)
		{ 
			//(y%32)*32
			return ((vu16*) MAP_BASE_ADR(bg1_base))[((ytile&31)<<5) + xtile + 2*32*32];
		}
	}
	else
	{
		if(ytile < 32) //upper right quadrant (1)
		{
			return ((vu16*) MAP_BASE_ADR(bg1_base))[(ytile<<5) + (xtile&31) + 32*32];
		}
		else //lower right quadrant (3)
		{
			return ((vu16*) MAP_BASE_ADR(bg1_base))[((ytile&31)<<5)+(xtile&31) + 3*32*32];
		}
	}
	return 0;
}

// Movement

bool IsKeyUp()
{
	int output = REG_KEYINPUT & KEY_UP;
	return !(output);
}

bool IsKeyDown()
{
	int output = REG_KEYINPUT & KEY_DOWN;
	return !(output);
}

bool IsKeyRight()
{
	int output = REG_KEYINPUT & KEY_RIGHT;
	return !(output);
}

bool IsKeyLeft()
{
	int output = REG_KEYINPUT & KEY_LEFT;
	return !(output);
}

bool IsBTapped()
{
	bool pressed = !(REG_KEYINPUT & KEY_B); 
    bool tapped = pressed && !prevB;       
    prevB = pressed;                        
    return tapped;
}

void executeMovement()
{
	link.px = link.x;
	link.py = link.y;

	int up = 0;
	int right = 0;
	if(IsKeyUp())
	{
		up-= link.topSpeed;
	}
	if(IsKeyDown())
	{
		up+= link.topSpeed;
	}
	if(IsKeyRight())
	{
		right+= link.topSpeed;
	}
	if(IsKeyLeft())
	{
		right-= link.topSpeed;
	}


	if(up == 0)
	{
		if(right > 0)
		{
			link.dir = DIR_RIGHT;
		}
		else if(right < 0)
		{
			link.dir = DIR_LEFT;
		}
	}
	else if(up > 0)
	{
		if(right > 0)
		{
			link.dir = DIR_RIGHT;
		}
		else if(right < 0)
		{
			link.dir = DIR_LEFT;
		}
		else
		{
			link.dir = DIR_UP;
		}
	}
	else if(up < 0)
	{
		if(right > 0)
		{
			link.dir = DIR_RIGHT;
		}
		else if(right < 0)
		{
			link.dir = DIR_LEFT;
		}
		else
		{
			link.dir = DIR_DOWN;
		}
	}

	if(right != 0 || up != 0)
	{
		link.isMoving = true;
	}
	else
	{
		link.isMoving = false;
	}

	link.x += right;
	link.y += up;

	// update sprite in OAM
    SPRT[0].X = link.x;
    SPRT[0].Y = link.y;
}

// end movement

// Link

void animateLink()
{
	if(!link.isMoving)
	{
		link.frame = 0;
		return;
	}

	link.frameTimer++;
	
	if(link.frameTimer >= 10)
	{
		link.frameTimer = 0;
		link.frame++;

		if(link.frame > 3)
		{
			link.frame = 0;
		}
		u16 baseTile;
		switch(link.dir)
        {
            case DIR_UP:  		baseTile = 4;  break;
            case DIR_RIGHT:  	baseTile = 20;  break;
            case DIR_DOWN: 		baseTile = 36; break;
            case DIR_LEFT:    	baseTile = 52; break;
            default:        	baseTile = 4;  break;
        }

		SPRT[0].Character = (baseTile + link.frame*4) << 1;
	}
}

void linkCollide()
{
	link.x = link.px;
	link.y = link.py;
}

void setLinkX(u16 x)
{
	link.x = x;
	SPRT[0].X = link.x;
}

void setLinkY(u16 y)
{
	link.y = y;
	SPRT[0].Y = link.y;
}

// End Link

// Boomerangs

void updateBoomerangs()
{
    for(int i = 0; i < 10; i++)
    {
        Boomerang* b = &booms[i];
        int s = b->sprnum;

		// check active
        if(!b->active)
        {
			b->x = 240;
			b->y = 160;
        }

		// move
		b->x += b->vx;
		b->y += b->vy;

		// update OAM sprite
        SPRT[s].X = b->x;
        SPRT[s].Y = b->y;

    }
}

void throwBoomerang()
{
	int bNum = -1;
	for(int i = 0; i < 10; i++)
    {
        Boomerang* b = &booms[i];
		if(!b->active)
		{
			bNum = i;
			i = 10;
		}
	}

	if(bNum < 0) return;

	Boomerang* b = &booms[bNum];


    b->active = true;	// activate
    b->x = link.x + 8;  // start at Link's center
    b->y = link.y + 8;

    // set velocity based on direction
    switch(link.dir)
    {
        case DIR_UP:    b->vx = 0;  b->vy = 2; break;
        case DIR_DOWN:  b->vx = 0;  b->vy = -2; break;
        case DIR_LEFT:  b->vx = -2; b->vy = 0; break;
        case DIR_RIGHT: b->vx = 2;  b->vy = 0; break;
    }

}

void animateBoomerangs()
{
	for(int i = 0; i < 10; i++)
    {
        Boomerang* b = &booms[i];
		if(!b->active)
            continue;   // ignore inactive boomerangs

        // advance animation timer
        b->frameTimer++;

        if(b->frameTimer >= 8) // change every 8 frames
        {
            b->frameTimer = 0;
            b->frame++;

            if(b->frame > 3) 
                b->frame = 0;

            u16 baseTile = 132; 
            SPRT[b->sprnum].Character = (baseTile + b->frame*4) << 1;
        }
	}
}

//end Boomerangs

void processInputs()
{
	executeMovement();
	if(IsBTapped())
	{
		throwBoomerang();
	}

	updateBoomerangs();
	animateBoomerangs();
}

// Collision

void processCollisions()
{


	u16 w = 16;
	u16 h = 16;

	u16 x = link.x;
	u16 y = link.y;

	u16 tlX = x; 		// top-left X
	u16 tlY = y; 		// top-left Y

	u16 tmX = x + w/2;	// top-middle X		
	u16 tmY = y; 		// top-middle Y 

	u16 trX = x + w; 	// top-right X		
	u16 trY = y; 		// top-right Y 
	
	u16 mlX = x;		// middle-left X
	u16 mlY = y + w/2;	// middle-left Y

	u16 mrX = x + w;	// middle-right X
	u16 mrY = y + h/2;	// middle-right Y

	u16 blX = x;		// bottom-left X
	u16 blY = y + h;	// boys love Y

	u16 bmX = x + w/2;	// bottom-middle X
	u16 bmY = y + h;	// bottom-middle Y

	u16 brX = x + w;	// bottom-right X
	u16 brY = y + h;	// bottom-right Y



	// only collide if two points collide
	u16 topCols = 0;	//collisions on the top side
	u16 leftCols = 0;	//collisions on the left side
	u16 bottomCols = 0;	//collisions on the bottom side
	u16 rightCols = 0;	//collisions on the right side

	if(getForegroundTile(tlX, tlY) != 0)
	{
		topCols ++;
		leftCols ++;
	}
	if(getForegroundTile(tmX, tmY) != 0)
	{
		topCols ++;
	}
	if(getForegroundTile(trX, trY) != 0)
	{
		topCols ++;
		rightCols ++;
	}

	if(getForegroundTile(mlX, mlY) != 0)
	{
		leftCols ++;
	}
	if(getForegroundTile(mrX, mrY) != 0)
	{
		rightCols ++;
	}

	if(getForegroundTile(blX, blY) != 0)
	{
		leftCols ++;
		bottomCols ++;
	}
	if(getForegroundTile(bmX, bmY) != 0)
	{
		bottomCols ++;
	}
	if(getForegroundTile(brX, brY) != 0)
	{
		rightCols ++;
		bottomCols ++;
	}


	if(topCols >= 2 || rightCols >= 2 || bottomCols >= 2 || leftCols >= 2)
	{
		linkCollide();
	}	


	// boomerangs

	for(int i = 0; i < 10; i++)
    {
        Boomerang* b = &booms[i];
		
		if(getForegroundTile(b->x + 8, b->y +8) != 0)
		{
			b->active = false;
		}
	}

} 

// End Collision

// Scroll

void scrollUp()
{
	scrollPosY -= 160;
	setLinkY(link.y + 160);
}

void scrollRight()
{
	scrollPosX += 240;
	setLinkX(link.x - 240);
}

void scrollDown()
{
	scrollPosY += 160;
	setLinkY(link.y - 160);
}

void scrollLeft()
{
	scrollPosX -= 240;
	setLinkX(link.x + 240);
}

void processScroll()
{
	if ((link.x + 8) > 240 && scrollPosX < 480)
	{
		scrollRight();
	}
	else if ((link.x + 8) < 0 && scrollPosX < 480)
	{
		scrollLeft();
	}
	else if ((link.y + 8) < 0 && scrollPosY < 480)
	{
		scrollUp();
	}
	else if ((link.y + 8) > 160 && scrollPosY < 480)
	{
		scrollDown();
	}
}

// end Scroll

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

	initVars();

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

	setupLinkSprite();	
 
	setupBoomerangSprites();

	while (1) 
	{
		VBlankIntrWait(); // Wait until the next frame starts; Do not remove
		processInputs();
		processCollisions();
		processScroll();
		animateLink();
	}

}



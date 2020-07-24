/****************************************************************************
**	OrangeBot Project
*****************************************************************************
**        /
**       /
**      /
** ______ \
**         \
**          \
*****************************************************************************
**	Longan Nano Display Driver
*****************************************************************************
**  Development of the display class for the ST7735S LCD controller
**  interfaced to the SPI0 of the Risc-V Longan Nano board
****************************************************************************/

/****************************************************************************
**	INCLUDES
****************************************************************************/

//Longan Nano HAL
#include <gd32vf103.h>
//LED class
#include "longan_nano_led.hpp"
//Time class
#include "longan_nano_chrono.hpp"
//Display driver SPI0+DMA
#include "ST7735S_W160_H80_C16.hpp"

/****************************************************************************
**	NAMESPACES
****************************************************************************/

/****************************************************************************
**	DEFINES
****************************************************************************/

//forever
#define EVER (;;)

//Blocking sprite draw function
//#define TEST_BUSY_DRAW

#define TEST_DRAW_UPDATE

/****************************************************************************
**	MACROS
****************************************************************************/

/****************************************************************************
**	PROTOTYPES
****************************************************************************/

//Hardwired fixed math function that draws an Amiga demo ball
extern int amigaBall(int x_, int y_, int ph_);

/****************************************************************************
**	GLOBAL VARIABILES
****************************************************************************/

//16b frame buffer. 16b colors 5r6g5b.
uint16_t g_frame_buffer[Longan_nano::Display::Config::PIXEL_COUNT];

/****************************************************************************
**	FUNCTIONS
****************************************************************************/

/****************************************************************************
**	@brief main
**	main | void
****************************************************************************/
//! @return int |
//! @details Entry point of program
/***************************************************************************/

int main( void )
{
	//----------------------------------------------------------------
	//	VARS
	//----------------------------------------------------------------

	//Temp return
	bool f_ret;
	//Display Driver
	Longan_nano::Display g_display;
    //Physics of the ball
    int px = 0;
    int py = 0;
    int dx = 1 << 8;
    int dy = 1 << 8;
    int ph = 0;

	//----------------------------------------------------------------
	//	INIT
	//----------------------------------------------------------------

	//Initialize LEDs
	Longan_nano::Leds::init();
    Longan_nano::Leds::set_color( Longan_nano::Leds::Color::BLACK );
	//Initialize the Display
    g_display.init();

	//----------------------------------------------------------------
	//	BODY
	//----------------------------------------------------------------

    for EVER
    {
        //Local pointer to global frame buffer
        uint16_t* pfb = g_frame_buffer;
        //Fast counters
        int tx, ty;
        //temp vars
        int fx, fy ,c;

        //For: Scan rows
        for (ty=0; ty < Longan_nano::Display::Config::HEIGHT; ty++)
        {
            //For: scan cols
            for (tx=0; tx <  Longan_nano::Display::Config::WIDTH; tx++)
            {
                fx = (tx << 8) - px;
                fy = (ty << 8) - py;
                c = amigaBall(fx, fy, ph);
                if (c >= 0)
                {
                    *pfb++ = c;
                }
                else
                {
                    *pfb++ = 0; 
                }
            }   //End for: Scan cols
        }   //End for: Scan rows

        #ifdef TEST_BUSY_DRAW
        {
            g_display.draw_sprite( 0, 0, Longan_nano::Display::Config::WIDTH, Longan_nano::Display::Config::HEIGHT, g_frame_buffer );
        }
        #endif

        #ifdef TEST_DRAW_UPDATE
        {
            //Register the sprite to be drawn. Authorize the update FSM to draw the sprite through the "update_sprite" non blocking method
            g_display.register_sprite( 0, 0, Longan_nano::Display::Config::WIDTH, Longan_nano::Display::Config::HEIGHT, g_frame_buffer );
            //Keep updating until the sprite is rendered. The function is non blocking to allow the main to do work
            //In this demo I wait until update is done rendering the sprite
            do
            {
                //Ask the display driver FSM to execute a step of the update. false = IDLE | true = BUSY
                f_ret = g_display.update_sprite(); 
                //If: busy
                if (f_ret== true)
                {
                    //Light up green LED
                    Longan_nano::Leds::set(Longan_nano::Leds::Color::GREEN);
                }
                //If: idle
                else
                {
                    Longan_nano::Leds::clear(Longan_nano::Leds::Color::GREEN);
                }
            } while (f_ret == true);
        }
        #endif

        //Toggle the RED led each frame rendered
        Longan_nano::Leds::toggle( Longan_nano::Leds::Color::RED );

        //----------------------------------------------------------------
        //	BALL PHYSICS
        //----------------------------------------------------------------

        //Physics of the ball
        px += dx; py += dy;
        if (dx > 0) ph += (1 << 8); else ph -= (1 << 8);         // Rotation.
        if (py > (16<<8))    { py = (16<<9) - py;    dy = -dy; } // Floor.
        if (px > (96<<8))    { px = (96<<9) - px;    dx = -dx; } // Right wall.
        if (px < (-64 << 8)) { px = (-64 << 9) - px; dx = -dx; } // Left wall.
        dy += 1 << 4; // Apply gravity.

    } //End forever

	//----------------------------------------------------------------
	//	RETURN
	//----------------------------------------------------------------

    return 0;
}	//end function: main

/***************************************************************************/
//!	@brief function
//!	amiga_ball |
/***************************************************************************/
//! @return int |
//! @details
//!	Renders the amiga ball on the frame buffer
//! Uses a fixed function to achieve that (no general purpose geometry and textures)
/***************************************************************************/

int w_approx(int x)
{
    // Generated in Python:
    // for i in range(32):
    //     print(int(8192 / ((1 - (i/32.0)**2)**0.5)))
    // print(65535)
    static const int s_lut[33] = {
         8192,  8196,  8208,  8228,  8256,  8293,  8339,  8395,  8460,  8536,
         8623,  8723,  8836,  8965,  9110,  9273,  9459,  9669,  9908, 10180,
        10494, 10856, 11280, 11782, 12385, 13123, 14052, 15262, 16921, 19378,
        23541, 33027, 65535
    };

    if (x < 0)
        x = -x;
    if (x > (32<<8) - 1)
        x = (32<<8) - 1;

    int m = x >> 8;
    int f = x & 0xff;
    int v0 = s_lut[m];
    int v1 = s_lut[m+1];
    return ((v0 * (0xff - f)) + (v1 * f)) >> 8;
}

int approx_asin(int x)
{
    x = (x * 11) >> 3;
    int x2 = (x*x)>>13;
    int x3 = (x2*x)>>13;
    int x5 = (x2*x3)>>13;
    x += (1365*x3 + 614*x5) >> 13;
    return x;
}

int c_remap(int x)
{
    if (x < 256)
        return 256-x;
    if (x < 4096)
        return 0;
    if (x < 4096+256)
        return x-4096;
    return 256;
}

int amigaBall(int x_, int y_, int ph_)
{
    x_ >>= 1;
    y_ >>= 1;
    x_ -= 32 << 8;
    y_ -= 32 << 8;

    int x = ( 8028 * x_ + 1627 * y_) >> 13;
    int y = (-1627 * x_ + 8028 * y_) >> 13;

    int r = x*x+y*y;
    if (r > (1<<26))
        return -1;

    x = (x * w_approx(y)) >> 13;
    y = approx_asin(y);
    x = approx_asin(x) + ph_;

    x &= 0x1fff;
    y &= 0x1fff;

    int cx = c_remap(x);
    int cy = c_remap(y);
    int cc = (((cx - 128) * (cy - 128)) >> 7) + 128;

    int R = 0x1f;
    int G = (0x3f * cc) >> 8;
    int B = (0x1f * cc) >> 8;

    cc = 256 - c_remap((1<<12) - (r >> 14));
    R = (R * cc) >> 8;
    G = (G * cc) >> 8;
    B = (B * cc) >> 8;

    return (R<<11) + (G<<5) + B;
}

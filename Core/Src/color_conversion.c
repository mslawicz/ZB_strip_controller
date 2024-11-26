#include "color_conversion.h"
#include <math.h>

//convert color data from xy space to RGB value
RGB_t convert_XY_to_RGB(XY_t color_xy)
{
#define constrain_from_0(x)		if(x < 0) { x = 0.0; }
	float gamma_correction(float  vat2correct);

    RGB_t color_rgb;

	const float ZXY2RGB[3][3] =	//matrix for converting Ikea light bulb color XY ( CIE 1931 colorspace ) to RGB
	{
			{ 1.656, -0.355, -0.255 },
			{ -0.707, 1.655, 0.036 },
			{ 0.052, -0.121, 1.012 }
	};
	const float XY_NORM = 1.0 / 65536.0;

	/* normalize x y z to 0..1 range */
	const float x_n = color_xy.X * XY_NORM;
	const float y_n = color_xy.Y * XY_NORM;
	const float z_n = 1.0 - x_n - y_n;

	/* calculate CIE X Y Z values */
	const float Y = 1.0;	//brightness value for calculations
	const float X = Y / y_n * x_n;
	const float Z = Y / y_n * z_n;

	/* calculate r g b color values (not normalized) */
	float r = X * ZXY2RGB[0][0] + Y * ZXY2RGB[0][1] + Z * ZXY2RGB[0][2];
	float g = X * ZXY2RGB[1][0] + Y * ZXY2RGB[1][1] + Z * ZXY2RGB[1][2];
	float b = X * ZXY2RGB[2][0] + Y * ZXY2RGB[2][1] + Z * ZXY2RGB[2][2];

	/* find maximum of r g b values */
	float rgb_max = r > g ? r : g;
	rgb_max = b > rgb_max ? b : rgb_max;

	/* normalize r g b to 0..1 range */
	if(rgb_max > 1.0)
	{
		r /= rgb_max;
		g /= rgb_max;
		b /= rgb_max;
	}

	constrain_from_0(r)
	constrain_from_0(g)
	constrain_from_0(b)

	/* apply gamma correction */
	r = gamma_correction(r);
	g = gamma_correction(g);
	b = gamma_correction(b);

	/* normalize to 0..255 */
	color_rgb.R = (uint8_t)(r * 255 + 0.5);
	color_rgb.G = (uint8_t)(g * 255 + 0.5);
	color_rgb.B = (uint8_t)(b * 255 + 0.5);

    return color_rgb;
}

//adjust gamma correction to a color value
float gamma_correction(float val2correct)
{
	if(val2correct <= 0.0031308)
	{
		return val2correct * 12.92;
	}

	return 1.055 * powf(val2correct, 0.416666) - 0.055;
}

//convert color data from HS space to RGB value
RGB_t convert_HS_to_RGB(HS_t color_hs)
{
#define HS_NUMB_SECTORS 3       /* number of sectors in the hue range */
    RGB_t color_rgb;
    static const RGB_t RGB_nodes[HS_NUMB_SECTORS + 1] =
    {
        {0xFF, 0, 0},   /* red */
        {0, 0xFF, 0},   /* green */
        {0, 0, 0xFF},   /* blue */
        {0xFF, 0, 0}    /* red again */
    };
    static const uint8_t Max_hue = 254;     //max value of hue in calculations
    static const uint8_t Hue_sector_size = (Max_hue + 1) / HS_NUMB_SECTORS;  //size of the hue sector

    uint8_t hue = (color_hs.hue > Max_hue) ? Max_hue : color_hs.hue;    //hue in range <0,254>
    uint8_t idx = hue / Hue_sector_size;  // index of hue sector <0,2>
    uint8_t hue_sect = hue % Hue_sector_size;  // hue value in a sector <0,HueSectorSize-1>
    uint8_t saturation_floor = (0xFF - color_hs.sat) >> 1;     //saturation-derived component of RGB values
    // calculate RGB components from hue and saturation and RGB node array
    color_rgb.R = (RGB_nodes[idx].R + (RGB_nodes[idx + 1].R - RGB_nodes[idx].R) * hue_sect / Hue_sector_size) * color_hs.sat / 0xFF + saturation_floor;
    color_rgb.G = (RGB_nodes[idx].G + (RGB_nodes[idx + 1].G - RGB_nodes[idx].G) * hue_sect / Hue_sector_size) * color_hs.sat / 0xFF + saturation_floor;
    color_rgb.B = (RGB_nodes[idx].B + (RGB_nodes[idx + 1].B - RGB_nodes[idx].B) * hue_sect / Hue_sector_size) * color_hs.sat / 0xFF + saturation_floor;
    return color_rgb;
}

//convert color temperature in mireds to Ikea light bulb color XY ( CIE 1931 colorspace )
XY_t convert_temp_to_XY(uint16_t color_temp)
{
	XY_t color_xy;
	color_xy.X = (uint16_t)(-0.0333105f * color_temp * color_temp + 75.92069f * color_temp + 10641.276f);
	color_xy.Y = (uint16_t)(-0.0862128f * color_temp * color_temp + 66.42181f * color_temp + 12830.694f);
	return color_xy;
}
#include "color_chart.hpp"
#include "graphics.hpp"
#include "raster.hpp"

namespace graphics {

const SDL_Color& color_black()
{
	static SDL_Color res = {0,0,0,255};
	return res;
}
	
const SDL_Color& color_white()
{
	static SDL_Color res = {0xFF,0xFF,0xFF,255};
	return res;
}
	
const SDL_Color& color_red()
{
	static SDL_Color res = {0xFF,0,0,255};
	return res;
}
	
const SDL_Color& color_green()
{
	static SDL_Color res = {0,0xFF,0,255};
	return res;
}
	
const SDL_Color& color_blue()
{
	static SDL_Color res = {0,0,0xFF,255};
	return res;
}
	
const SDL_Color& color_yellow()
{
	static SDL_Color res = {0xFF,0xFF,0,255};
	return res;
}

const SDL_Color& color_grey()
{
	static SDL_Color res = {0x80,0x80,0x80,255};
	return res;
}

const SDL_Color& color_snow()
{
	static SDL_Color res = {0xff, 0xfa, 0xfa, 0xff};
	return res;
}

const SDL_Color& color_snow_2()
{
	static SDL_Color res = {0xee, 0xe9, 0xe9, 0xff};
	return res;
}

const SDL_Color& color_snow_3()
{
	static SDL_Color res = {0xcd, 0xc9, 0xc9, 0xff};
	return res;
}

const SDL_Color& color_snow_4()
{
	static SDL_Color res = {0x8b, 0x89, 0x89, 0xff};
	return res;
}

const SDL_Color& color_ghost_white()
{
	static SDL_Color res = {0xf8, 0xf8, 0xff, 0xff};
	return res;
}

const SDL_Color& color_white_smoke()
{
	static SDL_Color res = {0xf5, 0xf5, 0xf5, 0xff};
	return res;
}

const SDL_Color& color_gainsboro()
{
	static SDL_Color res = {0xdc, 0xcd, 0x0c, 0xff};
	return res;
}

const SDL_Color& color_floral_white()
{
	static SDL_Color res = {0xff, 0xfa, 0xf0, 0xff};
	return res;
}

const SDL_Color& color_old_lace()
{
	static SDL_Color res = {0xfd, 0xf5, 0xe6, 0xff};
	return res;
}

const SDL_Color& color_linen()
{
	static SDL_Color res = {0xfa, 0xf0, 0xe6, 0xff};
	return res;
}

const SDL_Color& color_antique_white()
{
	static SDL_Color res = {0xfa, 0xeb, 0xd7, 0xff};
	return res;
}

const SDL_Color& color_antique_white_2()
{
	static SDL_Color res = {0xee, 0xdf, 0xcc, 0xff};
	return res;
}

const SDL_Color& color_antique_white_3()
{
	static SDL_Color res = {0xcd, 0xc0, 0xb0, 0xff};
	return res;
}

const SDL_Color& color_antique_white_4()
{
	static SDL_Color res = {0x8b, 0x83, 0x78, 0xff};
	return res;
}

const SDL_Color& color_papaya_whip()
{
	static SDL_Color res = {0xff, 0xef, 0xd5, 0xff};
	return res;
}

const SDL_Color& color_blanched_almond()
{
	static SDL_Color res = {0xff, 0xeb, 0xcd, 0xff};
	return res;
}

const SDL_Color& color_bisque()
{
	static SDL_Color res = {0xff, 0xe4, 0xc4, 0xff};
	return res;
}

const SDL_Color& color_bisque_2()
{
	static SDL_Color res = {0xee, 0xd5, 0xb7, 0xff};
	return res;
}

const SDL_Color& color_bisque_3()
{
	static SDL_Color res = {0xcd, 0xb7, 0x9e, 0xff};
	return res;
}

const SDL_Color& color_bisque_4()
{
	static SDL_Color res = {0x8b, 0x7d, 0x6b, 0xff};
	return res;
}

const SDL_Color& color_peach_puff()
{
	static SDL_Color res = {0xff, 0xda, 0xb9, 0xff};
	return res;
}

const SDL_Color& color_peach_puff_2()
{
	static SDL_Color res = {0xee, 0xcb, 0xad, 0xff};
	return res;
}

const SDL_Color& color_peach_puff_3()
{
	static SDL_Color res = {0xcd, 0xaf, 0x95, 0xff};
	return res;
}

const SDL_Color& color_peach_puff_4()
{
	static SDL_Color res = {0x8b, 0x77, 0x65, 0xff};
	return res;
}

const SDL_Color& color_navajo_white()
{
	static SDL_Color res = {0xff, 0xde, 0xad, 0xff};
	return res;
}

const SDL_Color& color_moccasin()
{
	static SDL_Color res = {0xff, 0xe4, 0xb5, 0xff};
	return res;
}

const SDL_Color& color_cornsilk()
{
	static SDL_Color res = {0xff, 0xf8, 0xdc, 0xff};
	return res;
}

const SDL_Color& color_cornsilk_2()
{
	static SDL_Color res = {0xee, 0xe8, 0xdc, 0xff};
	return res;
}

const SDL_Color& color_cornsilk_3()
{
	static SDL_Color res = {0xcd, 0xc8, 0xb1, 0xff};
	return res;
}

const SDL_Color& color_cornsilk_4()
{
	static SDL_Color res = {0x8b, 0x88, 0x78, 0xff};
	return res;
}

const SDL_Color& color_ivory()
{
	static SDL_Color res = {0xff, 0xff, 0xf0, 0xff};
	return res;
}

const SDL_Color& color_ivory_2()
{
	static SDL_Color res = {0xee, 0xee, 0xe0, 0xff};
	return res;
}

const SDL_Color& color_ivory_3()
{
	static SDL_Color res = {0xcd, 0xcd, 0xc1, 0xff};
	return res;
}

const SDL_Color& color_ivory_4()
{
	static SDL_Color res = {0x8b, 0x8b, 0x83, 0xff};
	return res;
}

const SDL_Color& color_lemon_chiffon()
{
	static SDL_Color res = {0xff, 0xfa, 0xcd, 0xff};
	return res;
}

const SDL_Color& color_seashell()
{
	static SDL_Color res = {0xff, 0xf5, 0xee, 0xff};
	return res;
}

const SDL_Color& color_seashell_2()
{
	static SDL_Color res = {0xee, 0xe5, 0xde, 0xff};
	return res;
}

const SDL_Color& color_seashell_3()
{
	static SDL_Color res = {0xcd, 0xc5, 0xbf, 0xff};
	return res;
}

const SDL_Color& color_seashell_4()
{
	static SDL_Color res = {0x8b, 0x86, 0x82, 0xff};
	return res;
}

const SDL_Color& color_honeydew()
{
	static SDL_Color res = {0xf0, 0xff, 0xf0, 0xff};
	return res;
}

const SDL_Color& color_honeydew_2()
{
	static SDL_Color res = {0xe0, 0xee, 0xe0, 0xff};
	return res;
}

const SDL_Color& color_honeydew_3()
{
	static SDL_Color res = {0xc1, 0xcd, 0xc1, 0xff};
	return res;
}

const SDL_Color& color_honeydew_4()
{
	static SDL_Color res = {0x83, 0x8b, 0x83, 0xff};
	return res;
}

const SDL_Color& color_mint_cream()
{
	static SDL_Color res = {0xf5, 0xff, 0xfa, 0xff};
	return res;
}

const SDL_Color& color_azure()
{
	static SDL_Color res = {0xf0, 0xff, 0xff, 0xff};
	return res;
}

const SDL_Color& color_alice_blue()
{
	static SDL_Color res = {0xf0, 0xf8, 0xff, 0xff};
	return res;
}

const SDL_Color& color_lavender()
{
	static SDL_Color res = {0xe6, 0xe6, 0xfa, 0xff};
	return res;
}

const SDL_Color& color_lavender_blush()
{
	static SDL_Color res = {0xff, 0xf0, 0xf5, 0xff};
	return res;
}

const SDL_Color& color_misty_rose()
{
	static SDL_Color res = {0xff, 0xe4, 0xe1, 0xff};
	return res;
}

const SDL_Color& color_dark_slate_gray()
{
	static SDL_Color res = {0x2f, 0x4f, 0x4f, 0xff};
	return res;
}

const SDL_Color& color_dim_gray()
{
	static SDL_Color res = {0x69, 0x69, 0x69, 0xff};
	return res;
}

const SDL_Color& color_slate_gray()
{
	static SDL_Color res = {0x70, 0x80, 0x90, 0xff};
	return res;
}

const SDL_Color& color_light_slate_gray()
{
	static SDL_Color res = {0x77, 0x88, 0x99, 0xff};
	return res;
}

const SDL_Color& color_gray()
{
	static SDL_Color res = {0xbe, 0xbe, 0xbe, 0xff};
	return res;
}

const SDL_Color& color_light_gray()
{
	static SDL_Color res = {0xd3, 0xd3, 0xd3, 0xff};
	return res;
}

const SDL_Color& color_midnight_blue()
{
	static SDL_Color res = {0x19, 0x19, 0x70, 0xff};
	return res;
}

const SDL_Color& color_navy()
{
	static SDL_Color res = {0x80, 0x00, 0x00, 0xff};
	return res;
}

const SDL_Color& color_cornflower_blue()
{
	static SDL_Color res = {0x64, 0x95, 0xed, 0xff};
	return res;
}

const SDL_Color& color_dark_slate_blue()
{
	static SDL_Color res = {0x48, 0x3d, 0x8b, 0xff};
	return res;
}

const SDL_Color& color_slate_blue()
{
	static SDL_Color res = {0x6a, 0x5a, 0xcd, 0xff};
	return res;
}

const SDL_Color& color_medium_slate_blue()
{
	static SDL_Color res = {0x7b, 0x68, 0xee, 0xff};
	return res;
}

const SDL_Color& color_light_slate_blue()
{
	static SDL_Color res = {0x84, 0x70, 0xff, 0xff};
	return res;
}

const SDL_Color& color_medium_blue()
{
	static SDL_Color res = {0x00, 0x00, 0xcd, 0xff};
	return res;
}

const SDL_Color& color_royal_blue()
{
	static SDL_Color res = {0x41, 0x69, 0x00, 0xff};
	return res;
}

const SDL_Color& color_dodger_blue()
{
	static SDL_Color res = {0x1e, 0x90, 0xff, 0xff};
	return res;
}

const SDL_Color& color_deep_sky_blue()
{
	static SDL_Color res = {0x00, 0xbf, 0xff, 0xff};
	return res;
}

const SDL_Color& color_sky_blue()
{
	static SDL_Color res = {0x87, 0xce, 0xeb, 0xff};
	return res;
}

const SDL_Color& color_light_sky_blue()
{
	static SDL_Color res = {0x87, 0xce, 0xfa, 0xff};
	return res;
}

const SDL_Color& color_steel_blue()
{
	static SDL_Color res = {0x46, 0x82, 0xb4, 0xff};
	return res;
}

const SDL_Color& color_light_steel_blue()
{
	static SDL_Color res = {0xb0, 0xc4, 0xde, 0xff};
	return res;
}

const SDL_Color& color_light_blue()
{
	static SDL_Color res = {0xad, 0xd8, 0xe6, 0xff};
	return res;
}

const SDL_Color& color_powder_blue()
{
	static SDL_Color res = {0xb0, 0xe0, 0xe6, 0xff};
	return res;
}

const SDL_Color& color_pale_turquoise()
{
	static SDL_Color res = {0xaf, 0xee, 0xee, 0xff};
	return res;
}

const SDL_Color& color_dark_turquoise()
{
	static SDL_Color res = {0x00, 0xce, 0xd1, 0xff};
	return res;
}

const SDL_Color& color_medium_turquoise()
{
	static SDL_Color res = {0x48, 0xd1, 0xcc, 0xff};
	return res;
}

const SDL_Color& color_turquoise()
{
	static SDL_Color res = {0x40, 0xe0, 0xd0, 0xff};
	return res;
}

const SDL_Color& color_cyan()
{
	static SDL_Color res = {0x00, 0xff, 0xff, 0xff};
	return res;
}

const SDL_Color& color_light_cyan()
{
	static SDL_Color res = {0xe0, 0xff, 0xff, 0xff};
	return res;
}

const SDL_Color& color_cadet_blue()
{
	static SDL_Color res = {0x5f, 0x9e, 0xa0, 0xff};
	return res;
}

const SDL_Color& color_medium_aquamarine()
{
	static SDL_Color res = {0x66, 0xcd, 0xaa, 0xff};
	return res;
}

const SDL_Color& color_aquamarine()
{
	static SDL_Color res = {0x7f, 0xff, 0xd4, 0xff};
	return res;
}

const SDL_Color& color_dark_green()
{
	static SDL_Color res = {0x64, 0x00, 0x00, 0xff};
	return res;
}

const SDL_Color& color_dark_olive_green()
{
	static SDL_Color res = {0x55, 0x6b, 0x2f, 0xff};
	return res;
}

const SDL_Color& color_dark_sea_green()
{
	static SDL_Color res = {0x8f, 0xbc, 0x8f, 0xff};
	return res;
}

const SDL_Color& color_sea_green()
{
	static SDL_Color res = {0x2e, 0x8b, 0x57, 0xff};
	return res;
}

const SDL_Color& color_medium_sea_green()
{
	static SDL_Color res = {0x3c, 0xb3, 0x71, 0xff};
	return res;
}

const SDL_Color& color_light_sea_green()
{
	static SDL_Color res = {0x20, 0xb2, 0xaa, 0xff};
	return res;
}

const SDL_Color& color_pale_green()
{
	static SDL_Color res = {0x98, 0xfb, 0x98, 0xff};
	return res;
}

const SDL_Color& color_spring_green()
{
	static SDL_Color res = {0x00, 0xff, 0x7f, 0xff};
	return res;
}

const SDL_Color& color_lawn_green()
{
	static SDL_Color res = {0x7c, 0xfc, 0x00, 0xff};
	return res;
}

const SDL_Color& color_chartreuse()
{
	static SDL_Color res = {0x7f, 0xff, 0x00, 0xff};
	return res;
}

const SDL_Color& color_medium_spring_green()
{
	static SDL_Color res = {0x00, 0xfa, 0x9a, 0xff};
	return res;
}

const SDL_Color& color_green_yellow()
{
	static SDL_Color res = {0xad, 0xff, 0x2f, 0xff};
	return res;
}

const SDL_Color& color_lime_green()
{
	static SDL_Color res = {0x32, 0xcd, 0x32, 0xff};
	return res;
}

const SDL_Color& color_yellow_green()
{
	static SDL_Color res = {0x9a, 0xcd, 0x32, 0xff};
	return res;
}

const SDL_Color& color_forest_green()
{
	static SDL_Color res = {0x22, 0x8b, 0x22, 0xff};
	return res;
}

const SDL_Color& color_olive_drab()
{
	static SDL_Color res = {0x6b, 0x8e, 0x23, 0xff};
	return res;
}

const SDL_Color& color_dark_khaki()
{
	static SDL_Color res = {0xbd, 0xb7, 0x6b, 0xff};
	return res;
}

const SDL_Color& color_khaki()
{
	static SDL_Color res = {0xf0, 0xe6, 0x8c, 0xff};
	return res;
}

const SDL_Color& color_pale_goldenrod()
{
	static SDL_Color res = {0xee, 0xe8, 0xaa, 0xff};
	return res;
}

const SDL_Color& color_light_goldenrod_yellow()
{
	static SDL_Color res = {0xfa, 0xfa, 0xd2, 0xff};
	return res;
}

const SDL_Color& color_light_yellow()
{
	static SDL_Color res = {0xff, 0xff, 0xe0, 0xff};
	return res;
}

const SDL_Color& color_gold()
{
	static SDL_Color res = {0xff, 0xd7, 0x00, 0xff};
	return res;
}

const SDL_Color& color_light_goldenrod()
{
	static SDL_Color res = {0xee, 0xdd, 0x82, 0xff};
	return res;
}

const SDL_Color& color_goldenrod()
{
	static SDL_Color res = {0xda, 0xa5, 0x20, 0xff};
	return res;
}

const SDL_Color& color_dark_goldenrod()
{
	static SDL_Color res = {0xb8, 0x86, 0x0b, 0xff};
	return res;
}

const SDL_Color& color_rosy_brown()
{
	static SDL_Color res = {0xbc, 0x8f, 0x8f, 0xff};
	return res;
}

const SDL_Color& color_indian_red()
{
	static SDL_Color res = {0xcd, 0x5c, 0x5c, 0xff};
	return res;
}

const SDL_Color& color_saddle_brown()
{
	static SDL_Color res = {0x8b, 0x45, 0x13, 0xff};
	return res;
}

const SDL_Color& color_sienna()
{
	static SDL_Color res = {0xa0, 0x52, 0x2d, 0xff};
	return res;
}

const SDL_Color& color_peru()
{
	static SDL_Color res = {0xcd, 0x85, 0x3f, 0xff};
	return res;
}

const SDL_Color& color_burlywood()
{
	static SDL_Color res = {0xde, 0xb8, 0x87, 0xff};
	return res;
}

const SDL_Color& color_beige()
{
	static SDL_Color res = {0xf5, 0xf5, 0xdc, 0xff};
	return res;
}

const SDL_Color& color_wheat()
{
	static SDL_Color res = {0xf5, 0xde, 0xb3, 0xff};
	return res;
}

const SDL_Color& color_sandy_brown()
{
	static SDL_Color res = {0xf4, 0xa4, 0x60, 0xff};
	return res;
}

const SDL_Color& color_tan()
{
	static SDL_Color res = {0xd2, 0xb4, 0x8c, 0xff};
	return res;
}

const SDL_Color& color_chocolate()
{
	static SDL_Color res = {0xd2, 0x69, 0x1e, 0xff};
	return res;
}

const SDL_Color& color_firebrick()
{
	static SDL_Color res = {0xb2, 0x22, 0x22, 0xff};
	return res;
}

const SDL_Color& color_brown()
{
	static SDL_Color res = {0xa5, 0x2a, 0x2a, 0xff};
	return res;
}

const SDL_Color& color_dark_salmon()
{
	static SDL_Color res = {0xe9, 0x96, 0x7a, 0xff};
	return res;
}

const SDL_Color& color_salmon()
{
	static SDL_Color res = {0xfa, 0x80, 0x72, 0xff};
	return res;
}

const SDL_Color& color_light_salmon()
{
	static SDL_Color res = {0xff, 0xa0, 0x7a, 0xff};
	return res;
}

const SDL_Color& color_orange()
{
	static SDL_Color res = {0xff, 0xa5, 0x00, 0xff};
	return res;
}

const SDL_Color& color_dark_orange()
{
	static SDL_Color res = {0xff, 0x8c, 0x00, 0xff};
	return res;
}

const SDL_Color& color_coral()
{
	static SDL_Color res = {0xff, 0x7f, 0x50, 0xff};
	return res;
}

const SDL_Color& color_light_coral()
{
	static SDL_Color res = {0xf0, 0x80, 0x80, 0xff};
	return res;
}

const SDL_Color& color_tomato()
{
	static SDL_Color res = {0xff, 0x63, 0x47, 0xff};
	return res;
}

const SDL_Color& color_orange_red()
{
	static SDL_Color res = {0xff, 0x45, 0x00, 0xff};
	return res;
}

const SDL_Color& color_hot_pink()
{
	static SDL_Color res = {0xff, 0x69, 0xb4, 0xff};
	return res;
}

const SDL_Color& color_deep_pink()
{
	static SDL_Color res = {0xff, 0x14, 0x93, 0xff};
	return res;
}

const SDL_Color& color_pink()
{
	static SDL_Color res = {0xff, 0xc0, 0xcb, 0xff};
	return res;
}

const SDL_Color& color_light_pink()
{
	static SDL_Color res = {0xff, 0xb6, 0xc1, 0xff};
	return res;
}

const SDL_Color& color_pale_violet_red()
{
	static SDL_Color res = {0xdb, 0x70, 0x93, 0xff};
	return res;
}

const SDL_Color& color_maroon()
{
	static SDL_Color res = {0xb0, 0x30, 0x60, 0xff};
	return res;
}

const SDL_Color& color_medium_violet_red()
{
	static SDL_Color res = {0xc7, 0x15, 0x85, 0xff};
	return res;
}

const SDL_Color& color_violet_red()
{
	static SDL_Color res = {0xd0, 0x20, 0x90, 0xff};
	return res;
}

const SDL_Color& color_violet()
{
	static SDL_Color res = {0xee, 0x82, 0xee, 0xff};
	return res;
}

const SDL_Color& color_plum()
{
	static SDL_Color res = {0xdd, 0xa0, 0xdd, 0xff};
	return res;
}

const SDL_Color& color_orchid()
{
	static SDL_Color res = {0xda, 0x70, 0xd6, 0xff};
	return res;
}

const SDL_Color& color_medium_orchid()
{
	static SDL_Color res = {0xba, 0x55, 0xd3, 0xff};
	return res;
}

const SDL_Color& color_dark_orchid()
{
	static SDL_Color res = {0x99, 0x32, 0xcc, 0xff};
	return res;
}

const SDL_Color& color_dark_violet()
{
	static SDL_Color res = {0x94, 0x00, 0xd3, 0xff};
	return res;
}

const SDL_Color& color_blue_violet()
{
	static SDL_Color res = {0x8a, 0x2b, 0xe2, 0xff};
	return res;
}

const SDL_Color& color_purple()
{
	static SDL_Color res = {0xa0, 0x20, 0xf0, 0xff};
	return res;
}

const SDL_Color& color_medium_purple()
{
	static SDL_Color res = {0x93, 0x70, 0xdb, 0xff};
	return res;
}

const SDL_Color& color_thistle()
{
	static SDL_Color res = {0xd8, 0xbf, 0xd8, 0xff};
	return res;
}

}

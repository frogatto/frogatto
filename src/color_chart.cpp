#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "asserts.hpp"
#include "color_chart.hpp"

namespace graphics {

namespace {

std::map<std::string,boost::function<const SDL_Color&()> > get_color_cache()
{
	static std::map<std::string,boost::function<const SDL_Color&()> > color_cache;
	return color_cache;
}

void color_cache_init()
{
	get_color_cache()["black"] = boost::bind(graphics::color_black);
	get_color_cache()["white"] = boost::bind(graphics::color_white);
	get_color_cache()["red"] = boost::bind(graphics::color_red);
	get_color_cache()["green"] = boost::bind(graphics::color_green);
	get_color_cache()["blue"] = boost::bind(graphics::color_blue);
	get_color_cache()["yellow"] = boost::bind(graphics::color_yellow);
	get_color_cache()["grey"] = boost::bind(graphics::color_grey);
	get_color_cache()["snow"] = boost::bind(graphics::color_snow);
	get_color_cache()["snow_2"] = boost::bind(graphics::color_snow_2);
	get_color_cache()["snow_3"] = boost::bind(graphics::color_snow_3);
	get_color_cache()["snow_4"] = boost::bind(graphics::color_snow_4);
	get_color_cache()["ghost_white"] = boost::bind(graphics::color_ghost_white);
	get_color_cache()["white_smoke"] = boost::bind(graphics::color_white_smoke);
	get_color_cache()["gainsboro"] = boost::bind(graphics::color_gainsboro);
	get_color_cache()["floral_white"] = boost::bind(graphics::color_floral_white);
	get_color_cache()["old_lace"] = boost::bind(graphics::color_old_lace);
	get_color_cache()["linen"] = boost::bind(graphics::color_linen);
	get_color_cache()["antique_white"] = boost::bind(graphics::color_antique_white);
	get_color_cache()["antique_white_2"] = boost::bind(graphics::color_antique_white_2);
	get_color_cache()["antique_white_3"] = boost::bind(graphics::color_antique_white_3);
	get_color_cache()["antique_white_4"] = boost::bind(graphics::color_antique_white_4);
	get_color_cache()["papaya_whip"] = boost::bind(graphics::color_papaya_whip);
	get_color_cache()["blanched_almond"] = boost::bind(graphics::color_blanched_almond);
	get_color_cache()["bisque"] = boost::bind(graphics::color_bisque);
	get_color_cache()["bisque_2"] = boost::bind(graphics::color_bisque_2);
	get_color_cache()["bisque_3"] = boost::bind(graphics::color_bisque_3);
	get_color_cache()["bisque_4"] = boost::bind(graphics::color_bisque_4);
	get_color_cache()["peach_puff"] = boost::bind(graphics::color_peach_puff);
	get_color_cache()["peach_puff_2"] = boost::bind(graphics::color_peach_puff_2);
	get_color_cache()["peach_puff_3"] = boost::bind(graphics::color_peach_puff_3);
	get_color_cache()["peach_puff_4"] = boost::bind(graphics::color_peach_puff_4);
	get_color_cache()["navajo_white"] = boost::bind(graphics::color_navajo_white);
	get_color_cache()["moccasin"] = boost::bind(graphics::color_moccasin);
	get_color_cache()["cornsilk"] = boost::bind(graphics::color_cornsilk);
	get_color_cache()["cornsilk_2"] = boost::bind(graphics::color_cornsilk_2);
	get_color_cache()["cornsilk_3"] = boost::bind(graphics::color_cornsilk_3);
	get_color_cache()["cornsilk_4"] = boost::bind(graphics::color_cornsilk_4);
	get_color_cache()["ivory"] = boost::bind(graphics::color_ivory);
	get_color_cache()["ivory_2"] = boost::bind(graphics::color_ivory_2);
	get_color_cache()["ivory_3"] = boost::bind(graphics::color_ivory_3);
	get_color_cache()["ivory_4"] = boost::bind(graphics::color_ivory_4);
	get_color_cache()["lemon_chiffon"] = boost::bind(graphics::color_lemon_chiffon);
	get_color_cache()["seashell"] = boost::bind(graphics::color_seashell);
	get_color_cache()["seashell_2"] = boost::bind(graphics::color_seashell_2);
	get_color_cache()["seashell_3"] = boost::bind(graphics::color_seashell_3);
	get_color_cache()["seashell_4"] = boost::bind(graphics::color_seashell_4);
	get_color_cache()["honeydew"] = boost::bind(graphics::color_honeydew);
	get_color_cache()["honeydew_2"] = boost::bind(graphics::color_honeydew_2);
	get_color_cache()["honeydew_3"] = boost::bind(graphics::color_honeydew_3);
	get_color_cache()["honeydew_4"] = boost::bind(graphics::color_honeydew_4);
	get_color_cache()["mint_cream"] = boost::bind(graphics::color_mint_cream);
	get_color_cache()["azure"] = boost::bind(graphics::color_azure);
	get_color_cache()["alice_blue"] = boost::bind(graphics::color_alice_blue);
	get_color_cache()["lavender"] = boost::bind(graphics::color_lavender);
	get_color_cache()["lavender_blush"] = boost::bind(graphics::color_lavender_blush);
	get_color_cache()["misty_rose"] = boost::bind(graphics::color_misty_rose);
	get_color_cache()["dark_slate_gray"] = boost::bind(graphics::color_dark_slate_gray);
	get_color_cache()["dim_gray"] = boost::bind(graphics::color_dim_gray);
	get_color_cache()["slate_gray"] = boost::bind(graphics::color_slate_gray);
	get_color_cache()["light_slate_gray"] = boost::bind(graphics::color_light_slate_gray);
	get_color_cache()["gray"] = boost::bind(graphics::color_gray);
	get_color_cache()["light_gray"] = boost::bind(graphics::color_light_gray);
	get_color_cache()["midnight_blue"] = boost::bind(graphics::color_midnight_blue);
	get_color_cache()["navy"] = boost::bind(graphics::color_navy);
	get_color_cache()["cornflower_blue"] = boost::bind(graphics::color_cornflower_blue);
	get_color_cache()["dark_slate_blue"] = boost::bind(graphics::color_dark_slate_blue);
	get_color_cache()["slate_blue"] = boost::bind(graphics::color_slate_blue);
	get_color_cache()["medium_slate_blue"] = boost::bind(graphics::color_medium_slate_blue);
	get_color_cache()["light_slate_blue"] = boost::bind(graphics::color_light_slate_blue);
	get_color_cache()["medium_blue"] = boost::bind(graphics::color_medium_blue);
	get_color_cache()["royal_blue"] = boost::bind(graphics::color_royal_blue);
	get_color_cache()["dodger_blue"] = boost::bind(graphics::color_dodger_blue);
	get_color_cache()["deep_sky_blue"] = boost::bind(graphics::color_deep_sky_blue);
	get_color_cache()["sky_blue"] = boost::bind(graphics::color_sky_blue);
	get_color_cache()["light_sky_blue"] = boost::bind(graphics::color_light_sky_blue);
	get_color_cache()["steel_blue"] = boost::bind(graphics::color_steel_blue);
	get_color_cache()["light_steel_blue"] = boost::bind(graphics::color_light_steel_blue);
	get_color_cache()["light_blue"] = boost::bind(graphics::color_light_blue);
	get_color_cache()["powder_blue"] = boost::bind(graphics::color_powder_blue);
	get_color_cache()["pale_turquoise"] = boost::bind(graphics::color_pale_turquoise);
	get_color_cache()["dark_turquoise"] = boost::bind(graphics::color_dark_turquoise);
	get_color_cache()["medium_turquoise"] = boost::bind(graphics::color_medium_turquoise);
	get_color_cache()["turquoise"] = boost::bind(graphics::color_turquoise);
	get_color_cache()["cyan"] = boost::bind(graphics::color_cyan);
	get_color_cache()["light_cyan"] = boost::bind(graphics::color_light_cyan);
	get_color_cache()["cadet_blue"] = boost::bind(graphics::color_cadet_blue);
	get_color_cache()["medium_aquamarine"] = boost::bind(graphics::color_medium_aquamarine);
	get_color_cache()["aquamarine"] = boost::bind(graphics::color_aquamarine);
	get_color_cache()["dark_green"] = boost::bind(graphics::color_dark_green);
	get_color_cache()["dark_olive_green"] = boost::bind(graphics::color_dark_olive_green);
	get_color_cache()["dark_sea_green"] = boost::bind(graphics::color_dark_sea_green);
	get_color_cache()["sea_green"] = boost::bind(graphics::color_sea_green);
	get_color_cache()["medium_sea_green"] = boost::bind(graphics::color_medium_sea_green);
	get_color_cache()["light_sea_green"] = boost::bind(graphics::color_light_sea_green);
	get_color_cache()["pale_green"] = boost::bind(graphics::color_pale_green);
	get_color_cache()["spring_green"] = boost::bind(graphics::color_spring_green);
	get_color_cache()["lawn_green"] = boost::bind(graphics::color_lawn_green);
	get_color_cache()["chartreuse"] = boost::bind(graphics::color_chartreuse);
	get_color_cache()["medium_spring_green"] = boost::bind(graphics::color_medium_spring_green);
	get_color_cache()["green_yellow"] = boost::bind(graphics::color_green_yellow);
	get_color_cache()["lime_green"] = boost::bind(graphics::color_lime_green);
	get_color_cache()["yellow_green"] = boost::bind(graphics::color_yellow_green);
	get_color_cache()["forest_green"] = boost::bind(graphics::color_forest_green);
	get_color_cache()["olive_drab"] = boost::bind(graphics::color_olive_drab);
	get_color_cache()["dark_khaki"] = boost::bind(graphics::color_dark_khaki);
	get_color_cache()["khaki"] = boost::bind(graphics::color_khaki);
	get_color_cache()["pale_goldenrod"] = boost::bind(graphics::color_pale_goldenrod);
	get_color_cache()["light_goldenrod_yellow"] = boost::bind(graphics::color_light_goldenrod_yellow);
	get_color_cache()["light_yellow"] = boost::bind(graphics::color_light_yellow);
	get_color_cache()["gold"] = boost::bind(graphics::color_gold);
	get_color_cache()["light_goldenrod"] = boost::bind(graphics::color_light_goldenrod);
	get_color_cache()["goldenrod"] = boost::bind(graphics::color_goldenrod);
	get_color_cache()["dark_goldenrod"] = boost::bind(graphics::color_dark_goldenrod);
	get_color_cache()["rosy_brown"] = boost::bind(graphics::color_rosy_brown);
	get_color_cache()["indian_red"] = boost::bind(graphics::color_indian_red);
	get_color_cache()["saddle_brown"] = boost::bind(graphics::color_saddle_brown);
	get_color_cache()["sienna"] = boost::bind(graphics::color_sienna);
	get_color_cache()["peru"] = boost::bind(graphics::color_peru);
	get_color_cache()["burlywood"] = boost::bind(graphics::color_burlywood);
	get_color_cache()["beige"] = boost::bind(graphics::color_beige);
	get_color_cache()["wheat"] = boost::bind(graphics::color_wheat);
	get_color_cache()["sandy_brown"] = boost::bind(graphics::color_sandy_brown);
	get_color_cache()["tan"] = boost::bind(graphics::color_tan);
	get_color_cache()["chocolate"] = boost::bind(graphics::color_chocolate);
	get_color_cache()["firebrick"] = boost::bind(graphics::color_firebrick);
	get_color_cache()["brown"] = boost::bind(graphics::color_brown);
	get_color_cache()["dark_salmon"] = boost::bind(graphics::color_dark_salmon);
	get_color_cache()["salmon"] = boost::bind(graphics::color_salmon);
	get_color_cache()["light_salmon"] = boost::bind(graphics::color_light_salmon);
	get_color_cache()["orange"] = boost::bind(graphics::color_orange);
	get_color_cache()["dark_orange"] = boost::bind(graphics::color_dark_orange);
	get_color_cache()["coral"] = boost::bind(graphics::color_coral);
	get_color_cache()["light_coral"] = boost::bind(graphics::color_light_coral);
	get_color_cache()["tomato"] = boost::bind(graphics::color_tomato);
	get_color_cache()["orange_red"] = boost::bind(graphics::color_orange_red);
	get_color_cache()["hot_pink"] = boost::bind(graphics::color_hot_pink);
	get_color_cache()["deep_pink"] = boost::bind(graphics::color_deep_pink);
	get_color_cache()["pink"] = boost::bind(graphics::color_pink);
	get_color_cache()["light_pink"] = boost::bind(graphics::color_light_pink);
	get_color_cache()["pale_violet_red"] = boost::bind(graphics::color_pale_violet_red);
	get_color_cache()["maroon"] = boost::bind(graphics::color_maroon);
	get_color_cache()["medium_violet_red"] = boost::bind(graphics::color_medium_violet_red);
	get_color_cache()["violet_red"] = boost::bind(graphics::color_violet_red);
	get_color_cache()["violet"] = boost::bind(graphics::color_violet);
	get_color_cache()["plum"] = boost::bind(graphics::color_plum);
	get_color_cache()["orchid"] = boost::bind(graphics::color_orchid);
	get_color_cache()["medium_orchid"] = boost::bind(graphics::color_medium_orchid);
	get_color_cache()["dark_orchid"] = boost::bind(graphics::color_dark_orchid);
	get_color_cache()["dark_violet"] = boost::bind(graphics::color_dark_violet);
	get_color_cache()["blue_violet"] = boost::bind(graphics::color_blue_violet);
	get_color_cache()["purple"] = boost::bind(graphics::color_purple);
	get_color_cache()["medium_purple"] = boost::bind(graphics::color_medium_purple);
	get_color_cache()["thistle"] = boost::bind(graphics::color_thistle);
}

}

const SDL_Color& get_color_from_name(std::string name)
{
	if(get_color_cache().empty()) {
		color_cache_init();
	}
	std::map<std::string,boost::function<const SDL_Color&()> >::iterator it = get_color_cache().find(name);
	if(it != get_color_cache().end()) {
		return it->second();
	}
	ASSERT_LOG(false, "Color "" << name << "" not known!");
	return color_black();
}

const SDL_Color& color_black()
{
	static SDL_Color res = {0x00, 0x00, 0x00, 0xff};
	return res;
}

const SDL_Color& color_white()
{
	static SDL_Color res = {0xff, 0xff, 0xff, 0xff};
	return res;
}

const SDL_Color& color_red()
{
	static SDL_Color res = {0xff, 0x00, 0x00, 0xff};
	return res;
}

const SDL_Color& color_green()
{
	static SDL_Color res = {0x00, 0xff, 0x00, 0xff};
	return res;
}

const SDL_Color& color_blue()
{
	static SDL_Color res = {0x00, 0x00, 0xff, 0xff};
	return res;
}

const SDL_Color& color_yellow()
{
	static SDL_Color res = {0xff, 0xff, 0x00, 0xff};
	return res;
}

const SDL_Color& color_grey()
{
	static SDL_Color res = {0x80, 0x80, 0x80, 0xff};
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

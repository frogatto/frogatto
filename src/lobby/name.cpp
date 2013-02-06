#include "name.hpp"
#include "shared_data.hpp"

const char* const name_list[] = 
{
	"David",
	"Kristina",
	"Caroline",
	"Wanetta",
	"Melvin",
	"Jeniffer",
	"Lucrecia",
	"Kathryne",
	"Tajuana",
	"Rutha",
	"Burton",
	"Lauretta",
	"Stasia",
	"Jaye",
	"Chauncey",
	"Bryant",
	"Octavia",
	"Adam",
	"Bradly",
	"Anne",
	"Maya",
	"Lajuana",
	"Von",
	"Fredia",
	"Keiko",
	"Leland",
	"Marielle",
	"Leisa",
	"Gordon",
	"Cindie",
	"Maren",
	"Vicki",
	"Ninfa",
	"Alfredo",
	"Luisa",
	"Catrice",
	"Emmy",
	"Latia",
	"Tawny",
	"Rosanne",
	"Kamala",
	"Jackeline",
	"Thelma",
	"Carli",
	"Rich",
	"Felicia",
	"Ilene",
	"Paris",
	"Raeann",
	"Willis",
	"Chu",
	"Rosaria",
};

namespace name
{
	std::string generate_bot_name()
	{
		int name_list_size = sizeof(name_list)/sizeof(name_list[0]);
		return std::string(name_list[game_server::shared_data::make_session_id() % name_list_size]) + "Bot";
	}
}

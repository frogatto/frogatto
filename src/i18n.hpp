#define _(String) i18n::tr(String)

namespace i18n {

void init();

const std::string& tr(const std::string& msgid);
const std::string& get_locale();
void use_system_locale();
void set_locale(const std::string& l);
void load_translations();

// Retrieve the codes and names of available languages.
// The arguments will be cleared first.
void get_available_locales(std::vector<std::string>& codes, std::vector<std::string>& names);
}


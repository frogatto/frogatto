#define _(String) i18n::tr(String)

namespace i18n {

void init();

const std::string& tr(const std::string& msgid);
const std::string& get_locale();
void use_system_locale();
void set_locale(const std::string& l);
void load_translations();
}


#include <cassert>
#include <fstream>

#include "desktop-entry.h"

int main(int argc, char **argv) {
    assert(argc == 2);

    std::ifstream file(argv[1]);
    xdg::desktop_entry_spec::desktop_entry entry {file};

    [[maybe_unused]] auto type = entry.get_type();
    [[maybe_unused]] auto name = entry.get_name();

    [[maybe_unused]] auto name_de               = name.get("de");
    [[maybe_unused]] auto name_de_DE            = name.get("de_DE");
    [[maybe_unused]] auto name_de_DE_mod        = name.get("de_DE@Latn");
    [[maybe_unused]] auto name_de_DE_mod_failed = name.get("de_DE@nope");
    [[maybe_unused]] auto name_de_ZU            = name.get("de_ZU");
    [[maybe_unused]] auto name_de_AU            = name.get("de_AU");
    [[maybe_unused]] auto name_de_AU_mod        = name.get("de_AU@Latn");
    [[maybe_unused]] auto name_de_mod           = name.get("de@Latn");
    [[maybe_unused]] auto name_en               = name.get("en");

    [[maybe_unused]] auto generic_name = entry.get_generic_name();
    [[maybe_unused]] auto exec         = entry.get_exec();
    [[maybe_unused]] auto try_exec     = entry.get_try_exec();

    return 0;
}

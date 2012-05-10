#ifndef STATS_HPP_INCLUDED
#define STATS_HPP_INCLUDED

#include "variant.hpp"

void init_tables(const variant& doc);

void read_stats(const variant& doc);
variant write_stats();

void process_stats(const variant& doc);

variant get_stats(const std::string& version, const std::string& lvl);

#endif

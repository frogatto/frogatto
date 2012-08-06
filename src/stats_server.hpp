#ifndef STATS_HPP_INCLUDED
#define STATS_HPP_INCLUDED

#include "variant.hpp"

void init_tables(const variant& doc);
void init_tables_for_module(const std::string& module, const variant& doc);
variant get_tables_definition();

std::map<std::string, std::string> get_stats_errors();

void read_stats(const variant& doc);
variant write_stats();

void process_stats(const variant& doc);

variant get_stats(const std::string& version, const std::string& module, const std::string& module_version, const std::string& lvl);

#endif

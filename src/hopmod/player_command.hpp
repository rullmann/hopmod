#ifndef HOPMOD_PLAYER_COMMAND_HPP
#define HOPMOD_PLAYER_COMMAND_HPP

#include "scripting.hpp"
#include <vector>
#include <string>

std::vector<std::string> parse_player_command_line(const char *);
bool eval_player_command(int,const char *,const std::vector<std::string> &,fungu::script::env &);
bool process_player_command(int,const char *);
extern char command_prefix;
extern bool using_command_prefix;

#endif
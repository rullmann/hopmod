player_command_script("info")
player_command_script("uptime")
player_command_script("lpc")
player_command_script("players")
player_command_script("versus")
player_command_script("1on1")
player_command_script("cheater")
player_command_script("getcn")
player_command_script("admin")
player_command_script("master")
player_command_script("votekick")
player_command_script("playermsg")
	player_command_alias("privmsg", "playermsg")
	player_command_alias("pmsg",    "playermsg")
	player_command_alias("pm",      "playermsg")
player_command_script("invadmin")

player_command_script("givemaster", nil, "master")
player_command_script("mute",       nil, "master")
player_command_script("unmute",     nil, "master")
player_command_script("names",      nil, "master")
player_command_script("specall",    nil, "master")
player_command_script("unspecall",  nil, "master")
player_command_script("kick",       nil, "master")
player_command_script("warning",    nil, "master")
player_command_script("addbot",     nil, "master")

player_command_script("pause",      nil, "admin")
player_command_script("resume",     nil, "admin")
player_command_script("maxclients", nil, "admin")
player_command_script("reload",     nil, "admin")
player_command_script("changetime", nil, "admin")
	player_command_alias("ctime", "changetime")
player_command_script("motd",       nil, "admin")
player_command_script("group",      nil, "admin")
player_command_script("ban",        nil, "admin")
player_command_script("persist",    nil, "admin")
player_command_script("msg",        nil, "admin")
player_command_script("getbans",    nil, "admin")
player_command_script("unban",      nil, "admin")
player_command_script("permban",    nil, "admin")
player_command_script("eval",       nil, "admin")
player_command_script("nosd",       nil, "admin")
player_command_script("sd",         nil, "admin")

local eslmatch_commands = loadfile(player_command_filename("eslmatch"))()
player_command_function("insta", eslmatch_commands.insta_cmd)
player_command_function("effic", eslmatch_commands.effic_cmd)

log_unknown_player_commands()

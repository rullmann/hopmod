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
player_command_alias("pm",    "playermsg")
player_command_alias("pchat", "playermsg")
player_command_alias("pc",    "playermsg")
player_command_script("invadmin")
player_command_script("whoisonline")
player_command_script("specmsg")
player_command_alias("specchat", "specmsg")
player_command_alias("sm",       "specmsg")
player_command_alias("schat",    "specmsg")
player_command_alias("sc",       "specmsg")
player_command_script("invmaster")
player_command_script("changeteam")
player_command_alias("cteam", "changeteam")
player_command_alias("team",  "changeteam")
player_command_script("getmap")

player_command_script("givemaster",  nil, "master")
player_command_script("mute",        nil, "master")
player_command_script("unmute",      nil, "master")
player_command_script("mutespecs",   nil, "master")
player_command_script("unmutespecs", nil, "master")
player_command_script("names",       nil, "master")
player_command_script("specall",     nil, "master")
player_command_script("unspecall",   nil, "master")
player_command_script("kick",        nil, "master")
player_command_script("warning",     nil, "master")
player_command_alias("warn", "warning")
player_command_script("addbot",      nil, "master")
player_command_script("recorddemo",  nil, "master")
player_command_alias("demo", "recorddemo")

player_command_script("pause",       nil, "admin")
player_command_script("resume",      nil, "admin")
player_command_script("maxclients",  nil, "admin")
player_command_script("reload",      nil, "admin")
player_command_script("changetime",  nil, "admin")
player_command_alias("time",     "changetime")
player_command_alias("ctime",    "changetime")
player_command_alias("timeleft", "changetime")
player_command_script("motd",        nil, "admin")
player_command_script("group",       nil, "admin")
player_command_script("ban",         nil, "admin")
player_command_script("persist",     nil, "admin")
player_command_script("msg",         nil, "admin")
player_command_script("getbans",     nil, "admin")
player_command_script("unban",       nil, "admin")
player_command_script("permban",     nil, "admin")
player_command_script("eval",        nil, "admin")
player_command_script("nosd",        nil, "admin")
player_command_script("sd",          nil, "admin")
player_command_script("slay",        nil, "admin")
player_command_script("giveadmin",   nil, "admin")
player_command_script("forcespec",   nil, "admin")
player_command_alias("fspec", "forcespec")
player_command_script("unforcespec", nil, "admin")
player_command_alias("unfspec", "unforcespec")

local eslmatch_commands = loadfile(player_command_filename("eslmatch"))()
player_command_function("insta", eslmatch_commands.insta_cmd)
player_command_function("effic", eslmatch_commands.effic_cmd)

local mapchange_commands = loadfile(player_command_filename("mapchange"))()
player_command_function("changemap", mapchange_commands.changemap_cmd, "master")
player_command_function("forcemap", mapchange_commands.forcemap_cmd,   "admin")
player_command_alias("cmap", "changemap")
player_command_alias("fmap", "forcemap")

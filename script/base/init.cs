# Do not delete this file. The game server expects this script "script/init.cs"
# to exist. This is the first script to be executed by the game server.

// Some utility functions that should be in utils.cs
do = [arg1]
global execCubeScriptFile &exec-cubescript
global execIfFound (func [filename] [
    if (fileExists $filename) [exec $filename]
])

exec "./script/base/resetvars.cs"
exec "./script/base/module.lua"
exec "./script/base/utils.lua"
exec "./script/base/init.lua"
exec "./script/base/irc.csl"

execIfFound "./conf/server.conf"

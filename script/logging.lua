
local logfile = io.open("log/server.log","a+")

function log(msg)
    logfile:write(os.date("[%a %b %d %X] ",os.time()))
    logfile:write(msg)
    logfile:write("\n")
    logfile:flush()
end

server.log = log

server.event_handler("failedconnect", function(ip, reason)
    if reason == "normal" then reason = "client-side failure" end
    log(string.format("%s unable to connect: %s",ip, reason))
end)

server.event_handler("connect", function (cn)

    local ip = server.player_ip(cn)
    local country = server.ip_to_country(ip)
    
    log(string.format("%s(%i)(%s)(%s) connected",server.player_name(cn),cn,ip,country))
end)

server.event_handler("disconnect", function (cn,reason)
    
    local reason_tag = ""
    if reason ~= "normal" then reason_tag = "because: " .. reason end
    
    log(string.format("%s(%i) disconnected %s",server.player_name(cn),cn,reason_tag))
end)

server.event_handler("kick", function(cn, bantime, admin, reason)
    
    local reason_tag = ""
    if reason ~= "" then reason_tag = "for " .. reason end
    
    local action_tag = "kicked"
    if tonumber(bantime) < 0 then action_tag = "kicked and permanently banned" end
    
    log(string.format("%s(%i) was %s by %s %s",server.player_name(cn),cn,action_tag,admin,reason_tag))
end)

server.event_handler("rename",function(cn, oldname, newname)
    log(string.format("%s(%i) renamed to %s",oldname,cn,newname))
end)

server.event_handler("reteam",function(cn, oldteam, newteam)
    log(string.format("%s(%i) changed team to %s",server.player_name(cn),cn,newteam))
end)

server.event_handler("text", function(cn, msg)
    log(string.format("%s(%i): %s",server.player_name(cn),cn,msg))
end)

server.event_handler("sayteam", function(cn, msg)
    log(string.format("%s(%i)(team): %s",server.player_name(cn),cn,msg))
end)

server.event_handler("mapvote", function(cn, map, mode)
    log(string.format("%s(%i) suggests %s on map %s",server.player_name(cn),cn,mode,map))
end)

server.event_handler("setmastermode", function(oldmode, newmode)
    log(string.format("mastermode changed to %s",newmode))
end)

server.event_handler("setmaster", function(cn, priv, value)
    
    local action_tag = "claimed"
    if tonumber(value) == 0 then action_tag = "relinquished" end
    
    log(string.format("%s(%i) %s %s",server.player_name(cn),cn,action_tag,priv))
end)

server.event_handler("spectator", function(cn, value)
    
    local action_tag = "joined"
    if tonumber(value) == 0 then action_tag = "left" end
    
    log(string.format("%s(%i) %s spectators",server.player_name(cn),cn,action_tag))
end)

server.event_handler("auth", function(cn, authname, success)
    
    local action_tag = "passed"
    if tonumber(success) == 0 then action_tag = "failed" end
    
    log(string.format("%s(%i) %s authentication as %s",server.player_name(cn),cn,action_tag,authname))
end)


server.event_handler("gamepaused", function() log("game is paused") end)
server.event_handler("gameresumed", function() log("game is resumed") end)

server.event_handler("addbot", function(cn)
    log(string.format("%s(%i) added a bot",server.player_name(cn),cn))
end)

server.event_handler("delbot", function(cn)
    log(string.format("%s(%i) deleted a bot",server.player_name(cn),cn))
end)

server.event_handler("beginrecord", function(id,filename)
    log(string.format("recording game to %s",filename))
end)

server.event_handler("endrecord", function(id, size)
    log(string.format("finished recording game (%s file size)",format_filesize(tonumber(size))))
end)

server.event_handler("mapcrcfail", function(cn) log(string.format("%s(%i) has a modified map",server.player_name(cn),cn)) end)

server.event_handler("shutdown", function() log("server shutting down"); logfile:close() end)

log("server started")

-- #group [all] <tag> [<team>]

function server.playercmd_group(cn,arg1,arg2,arg3)
    return mastercmd(function()
        if not server.gamemodeinfo.teams and server.reassignteams == 1 then
            return
        end
        if not arg1 then
            server.player_msg(cn,red("#group [all] <tag> [<team>]"))
            return
        end
        local tag = nil
        local team = nil
        if arg1 == "all" then
            if not arg2 then
                server.player_msg(cn,red("missing tag argument"))
                return
            end
            tag = arg2
            if arg3 then
                team = arg3
            else
        	team = tag
            end
            for j,cn in ipairs(server.spectators()) do
                if string.find(server.player_name(cn),tag) then
                    server.unspec(cn)
                    server.changeteam(cn,team)
                end
            end
        else
            tag = arg1
            if arg2 then
                team = arg2
            else
        	team = tag
            end
        end
        for i, cn in ipairs(server.players()) do
            if string.find(server.player_name(cn),tag) then
                server.changeteam(cn,team)
            end
        end
    end,cn)
end

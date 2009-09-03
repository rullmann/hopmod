local cmd_givemaster = {}

function cmd_givemaster.playercmd(cn,target)
    if not server.valid_cn(target) then
        server.player_msg(cn, red("Player not found."))
        return
    end
    server.unsetmaster()
    server.player_msg(target, server.player_name(cn) .. " has passed master privilege to you.")
    server.setmaster(target)
end

if server.givemaster_command_master == 1 then
    function server.playercmd_givemaster(cn,target)
	return mastercmd(function()
    	    cmd_givemaster.playercmd(cn,target)
	end,cn)
    end
else
    function server.playercmd_givemaster(cn,target)
	return admincmd(function()
    	    cmd_givemaster.playercmd(cn,target)
	end,cn)
    end
end

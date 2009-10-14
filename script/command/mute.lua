--[[

	A player command to mute a player
    
]]

return function(cn,tcn)

    if not server.mute then
        return false, "muting is disabled at this time."
    end

	if not tcn then
		return false, "#mute <cn>|\"<name>\""
	end

	if not server.valid_cn(tcn) then

		tcn = server.name_to_cn_list_matches(cn,tcn)

		if not tcn then
			return
		end
	end

	server.mute(tcn)
end

-- #invadmin

local function invadmin_domainhandler(cn,name)
	f_setinv_admin(cn)
end

local function f_setinvadmin(cn)
    server.set_invadmin(cn)
    server.player_msg(cn,"(" .. green("Info") .. ") Your rights have been raised to invisible-admin.")
end

function server.playercmd_invadmin(cn, pw)
	if pw then
		if server.check_admin_password(pw) then
			f_setinvadmin(cn)
		end
	else
	    if server.player_priv_code(cn) > 0 then
			server.unset_invadmin(cn)
		else
			server.player_vars(cn).invadmin = server.invadmin_domain
			auth.sendauthreq(cn,server.invadmin_domain)
		end
	end
end

server.event_handler("started",function()
    auth.add_domain_handler(server.invadmin_domain,function(cn,name)
        if server.player_vars(cn).invadmin then
            if server.player_vars(cn).invadmin == server.invadmin_domain then
                server.player_vars(cn).invadmin = nil
                invadmin_domainhandler(cn,name)
            end
        end
    end)
end)

function server.playercmd_invadmin(cn)
	server.check_admin_password
end
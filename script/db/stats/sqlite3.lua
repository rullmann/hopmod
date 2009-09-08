require "sqlite3"
dofile("./script/db/sqliteutils.lua")

local db, perr, insert_game, insert_team, insert_player, select_player_totals

local function set_sqlite3_synchronous_pragma(db, value)
    
    local accepted = {[0] = true, [1] = true, [2] = true, ["OFF"] = true, ["NORMAL"] = true, ["FULL"] = true}
    
    if not accepted[value] then
        error("Unrecognised value set for stats_sqlite_synchronous variable.")
    end
    
    db:exec("PRAGMA synchronous = " .. value)
end

local function open(settings)
    
    db,perr = sqlite3.open(settings.filename)
    if not db then return nil, perr end
    
    createMissingTables(settings.schemafile, db)
    
    if settings.exclusive_locking == 1 then
        db:exec("PRAGMA locking_mode=EXCLUSIVE")
    end
    
    set_sqlite3_synchronous_pragma(db, settings.synchronous)
    
    insert_game,perr = db:prepare("INSERT INTO games (datetime, duration, gamemode, mapname, players, bots, finished) VALUES (:datetime, :duration, :mode, :map, :players, :bots, :finished)")
    if not insert_game then return nil, perr end

    insert_team,perr = db:prepare("INSERT INTO teams (game_id, name, score, win, draw) VALUES (:gameid,:name,:score,:win,:draw)")
    if not insert_team then return nil, perr end

    insert_player,perr = db:prepare[[INSERT INTO players (game_id, team_id, name, ipaddr, country, score, frags, deaths, suicides, teamkills, hits, shots, damage, damagewasted, timeplayed, finished, win, rank, botskill) 
        VALUES(:gameid, :team_id, :name, :ipaddr, :country, :score, :frags, :deaths, :suicides, :teamkills, :hits, :shots, :damage, :damagewasted, :timeplayed, :finished, :win, :rank, :botskill)]]
    if not insert_player then return nil, perr end

    select_player_totals,perr = db:prepare("SELECT * FROM playertotals WHERE name = :name")
    if not select_player_totals then error(perr) end
    
    server.stats_db_absolute_filename = server.PWD .. "/" .. settings.filename
    
    return true
end

local function commit_game(game, players)
    
    db:exec("BEGIN TRANSACTION")
    
    insert_game:bind(game)
    insert_game:exec()
    local game_id = db:last_insert_rowid()
    
    if server.gamemodeinfo.teams then
    
        for i, teamname in ipairs(server.teams()) do
            
            team = {}
            team.gameid = game_id
            team.name = teamname
            team.score = server.team_score(teamname)
            team.win = server.team_win(teamname)
            team.draw = server.team_draw(teamname)
            
            insert_team:bind(team)
            insert_team:exec()
            
            local team_id = db:last_insert_rowid()
            local team_name = team.name
            
            --for i2,teamplayer in ipairs(server.team_players(teamname)) do
            --    statsmod.getPlayerTable(server.player_id(teamplayer)).team_id = team_id
            --end
            
            for id, player in pairs(players) do
            
                if player.team == team_name then player.team_id = team_id end
            end
        end
    end
    
    for id, player in pairs(players) do
    
        player.gameid = game_id
        
        insert_player:bind(player)
        insert_player:exec()
    end
    
    db:exec("COMMIT TRANSACTION")
    
end

local function player_totals(name)
    select_player_totals:bind{name = name}
    row = select_player_totals:first_row()
    return row
end

return {open = open, commit_game = commit_game, player_totals = player_totals}

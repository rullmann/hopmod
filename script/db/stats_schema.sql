
CREATE TABLE games (
    id                  INTEGER PRIMARY KEY,
    datetime            INTEGER DEFAULT 0,
    gamemode            TEXT DEFAULT "",
    mapname             TEXT DEFAULT "",
    duration            INTEGER DEFAULT 0,
    players             INTEGER DEFAULT 0
);

CREATE TABLE teams (
    id                  INTEGER PRIMARY KEY,
    game_id             INTEGER REFERENCES games(id),
    name                TEXT,
    score               INTEGER DEFAULT 0,
    win                 BOOLEAN DEFAULT false,
    draw                BOOLEAN DEFAULT false
);

CREATE TABLE players (
    id                  INTEGER PRIMARY KEY,
    game_id             INTEGER REFERENCES games(id),
    team_id             INTEGER REFERENCES teams(id) DEFAULT 0,
    name                TEXT,
    ipaddr              TEXT,
    frags               INTEGER DEFAULT 0,
    deaths              INTEGER DEFAULT 0,
    suicides            INTEGER DEFAULT 0,
    teamkills           INTEGER DEFAULT 0,
    hits                INTEGER DEFAULT 0,
    shots               INTEGER DEFAULT 0,
    damage              INTEGER DEFAULT 0,
    timeplayed          INTEGER DEFAULT 0,
    finished            BOOLEAN DEFAULT false,
    win                 BOOLEAN DEFAULT false
);

CREATE TABLE playertotals (
    id                  INTEGER PRIMARY KEY,
    name                TEXT UNIQUE,
    ipaddr              TEXT,
    first_game          TEXT,
    last_game           TEXT,
    frags               INTEGER DEFAULT 0,
    deaths              INTEGER DEFAULT 0,
    suicides            INTEGER DEFAULT 0,
    teamkills           INTEGER DEFAULT 0,
    hits                INTEGER DEFAULT 0,
    shots               INTEGER DEFAULT 0,
    wins                INTEGER DEFAULT 0,
    losses              INTEGER DEFAULT 0,
    games               INTEGER DEFAULT 0,
    withdraws           INTEGER DEFAULT 0,
    timeplayed          INTEGER DEFAULT 0
);

CREATE INDEX "player_name" on players (name);
CREATE INDEX "player_ipaddr" on players (ipaddr);
CREATE INDEX "game_id" on players (game_id ASC);
CREATE INDEX "playertotals_by_name" on playertotals (name ASC);

CREATE TRIGGER delete_game_trigger AFTER DELETE ON games
BEGIN
    DELETE FROM teams where game_id = old.id;
    DELETE FROM players where game_id = old.id;
END;

CREATE TRIGGER delete_player_trigger AFTER DELETE ON players
BEGIN
    UPDATE playertotals SET
        frags = frags - old.frags,
        deaths = deaths - old.deaths,
        suicides = suicides - old.suicides,
        teamkills = teamkills - old.teamkills,
        hits = hits - old.hits,
        shots = shots - old.shots,
        wins = wins - old.win,
        losses = losses - (old.win = 0),
        games = games - 1,
        withdraws = withdraws - (old.finished = 0)
        WHERE name = old.name;
END;

CREATE TRIGGER update_playertotals_trigger AFTER INSERT ON players
BEGIN
    INSERT OR IGNORE INTO playertotals (name,first_game) VALUES (new.name,strftime('%s','now'));
    UPDATE playertotals SET 
        ipaddr = new.ipaddr,
        last_game = strftime('%s','now'), 
        frags = frags + new.frags, 
        deaths = deaths + new.deaths,
        suicides = suicides + new.suicides,
        teamkills = teamkills + new.teamkills,
        hits = hits + new.hits,
        shots = shots + new.shots,
        wins = wins + new.win,
        losses = losses + (new.win = 0),
        games = games + 1,
        withdraws = withdraws + (new.finished = 0),
        timeplayed = timeplayed + new.timeplayed
        WHERE name = new.name;
END;

# Default values for external variables

global motd "Running Hopmod"

global banlist_file "conf/bans"
global geoip_db_file "share/GeoIP.dat"

global use_script_socket_server 1
global script_socket_port 7894
global script_socket_password ""

global allow_mapvote 1
global allow_modevote 1
global mapvote_disallow_unknown_map 0

global auth_db_filename "log/auth.sqlite"
global auth_use_sqlite 1

global record_player_stats 1
global stats_db_filename "log/stats.sqlite"
global stats_use_auth 0
global stats_auth_domain ""
global stats_tell_auth_name 0
global stats_debug 0
global stats_use_sqlite 1
global stats_sqlite_exclusive_locking 0
global stats_sqlite_synchronous 1
global stats_use_json 0
global stats_overwrite_name_with_authname 0
global stats_use_mysql 0
global stats_mysql_hostname "localhost"
global stats_mysql_port 3306
global stats_mysql_database "sauerstats"
global stats_mysql_username ""
global stats_mysql_password ""

global use_name_reservation 0
global name_reservation_domain ""
global reserved_name_expire (mins 3600)

global use_server_maprotation 1
global use_server_random_maprotation 0
global use_server_random_moderotation 0

global disallow_coopedit 0

global ffa_maps [complex douze ot academy metl2 metl3 nmp8 refuge tartech
    kalking1 dock turbine fanatic_quake oddworld wake5 aard3c curvedm
    fragplaza pgdm kffa neondevastation hog2 memento neonpanic lostinspace
    DM_BS1 shindou sdm1 shinmei1 stemple powerplant phosgene oasis island
    metl4 ruby frozen ksauer1 killfactory corruption deathtek aqueducts orbe
    arabic roughinery shadowed torment konkuri-to moonlite darkdeath
    fanatic_castle_trap orion nmp10 katrez_d thor frostbyte ogrosupply kmap5
    thetowers guacamole tejen hades paradigm mechanic wdcd]

global small_ffa_maps [complex douze ot academy metl2 metl3 tartech fanatic_quake
    turbine oddworld aard3c kffa neondevastation hog2 memento neonpanic sdm1
    shinmei1 island metl4 frozen darkdeath orion nmp10 torment]

global big_ffa_maps [refuge kalking1 dock lostinspace DM_BS1 shindou wake5
    fragplaza pgdm frostbyte stemple powerplant killfactory corruption deathtek
    aqueducts orbe arabic ogrosupply curvedm ruby shadowed konkuri-to moonlite
    fanatic_castle_trap tejen katrez_d kmap5 thor thetowers hades paradigm
    mechanic wdcd nmp8 phosgene oasis ksauer1 roughinery guacamole]

global instagib_maps [complex douze ot academy metl2 metl3 nmp8 tartech kalking1
    dock turbine fanatic_quake oddworld wake5 aard3c curvedm kffa
    neondevastation hog2 neonpanic sdm1]

global small_instagib_maps [complex douze ot academy metl2 metl3 nmp8 tartech dock
    turbine oddworld aard3c kffa neondevastation hog2 memento neonpanic
    sdm1 shinmei1 island metl4 frozen]

global big_instagib_maps [refuge kalking1 fanatic_quake lostinspace DM_BS1 shindou
    stemple powerplant killfactory corruption deathtek aqueducts orbe]

global capture_maps [urban_c nevil_c fb_capture nmp9 c_valley lostinspace fc3
    face-capture nmp4 nmp8 hallo monastery ph-capture hades fc4 relic frostbyte
    venice river_c paradigm corruption asteroids ogrosupply reissen akroseum
    duomo capture_night c_egypt tejen dust2 campo killcore3 damnation arabic
    serenity cwcastle]

global small_capture_maps [nevil_c fb_capture nmp9 fc3 frostbyte corruption
    asteroids reissen capture_night tejen dust2 campo damnation cwcastle
    face-capture]

global ctf_maps [hallo reissen face-capture flagstone shipwreck urban_c dust2
    berlin_wall akroseum valhalla damnation mach2 redemption tejen europium
    capture_night l_ctf forge campo wdcd sacrifice core_transfer recovery
    frostbyte]

global small_ctf_maps [reissen face-capture damnation tejen capture_night l_ctf
    forge campo wdcd recovery]

global "teamplay_maps" &ffa_maps
global small_teamplay_maps &small_ffa_maps

global "efficiency_maps" &ffa_maps
global small_efficiency_maps &small_ffa_maps
global big_efficiency_maps &big_ffa_maps

global "efficiency team_maps" &ffa_maps
global "small_efficiency team_maps" &small_ffa_maps

global "tactics_maps" &ffa_maps
global small_tactics_maps &small_ffa_maps
global big_tactics_maps &big_ffa_maps

global "tactics team_maps" &ffa_maps
global "small_tactics team_maps" &small_ffa_maps

global "instagib team_maps" &instagib_maps
global "small_instagib team_maps" &small_instagib_maps

global "regen capture_maps" &capture_maps
global "small_regen capture_maps" &small_capture_maps

global "insta ctf_maps" &ctf_maps
global "small_insta ctf_maps" &small_ctf_maps

global "protect_maps" &ctf_maps
global small_protect_maps &small_ctf_maps

global "insta protect_maps" &ctf_maps
global "small_insta protect_maps" &small_ctf_maps

global game_modes [instagib efficiency tactics ffa
    "instagib team" "efficiency team" "tactics team" teamplay
    "insta ctf" ctf "insta protect" protect capture "regen capture"]

global def_ctf_maps [hallo reissen flagstone face-capture shipwreck dust2
    urban_c berlin_wall akroseum valhalla damnation mach2 redemption tejen
    europium capture_night l_ctf forge campo wdcd sacrifice core_transfer
    recovery]

global def_capture_maps [urban_c nevil_c fb_capture nmp9 c_valley lostinspace
    fc3 face-capture nmp4 nmp8 hallo monastery ph-capture hades fc4 relic
    frostbyte venice paradigm corruption asteriods ogrosupply reissen akroseum
    duomo capture_night c_egypt tejen dust2 campo killcore3 damnation arabic
    cwcastle river_c serenity]

global def_ffa_maps [complex douze ot academy metl2 metl3 nmp8 refuge tartech
    kalking1 dock turbine fanatic_quake oddworld wake5 aard3c curvedm fragplaza
    pgdm kffa neondevastation hog2 memento neonpanic lostinspace DM_BS1 shindou
    sdm1 shinmei1 stemple powerplant phosgene oasis island metl4 ruby frozen
    ksauer1 killfactory corruption deathtek aqueducts orbe roughinery shadowed
    torment konkuri-to moonlite darkdeath fanatic_castle_trap orion nmp10 katrez_d
    thor frostbyte ogrosupply kmap5 thetowers guacamole tejen hades paradigm mechanic
    wdcd]

global "def_teamplay_maps" &def_ffa_maps
global "def_efficiency_maps" &def_ffa_maps
global "def_efficiency team_maps" &def_ffa_maps
global "def_tactics_maps" &def_ffa_maps
global "def_tactics team_maps" &def_ffa_maps
global "def_instagib_maps" &def_ffa_maps
global "def_instagib team_maps" &def_ffa_maps
global "def_regen capture_maps" &def_capture_maps
global "def_insta ctf_maps" &def_ctf_maps
global "def_protect_maps" &def_ctf_maps
global "def_insta protect_maps" &def_ctf_maps

global def_game_modes [ffa teamplay instagib "instagib team" efficiency
    "efficiency team" tactics "tactics team" capture "regen capture" ctf
    "insta ctf" protect "insta protect"]

global use_best_map_size 0
global small_gamesize 5
global small_teamgamesize 5

global first_map "complex"
global first_gamemode "ffa"
global firstgame_on_empty 0
global random_map_on_empty 0
global random_mode_on_empty 0

global use_irc_bot 0

flood_protect_text 1000
flood_protect_sayteam 1000
flood_protect_mapvote 1000
flood_protect_switchteam 1000
flood_protect_switchname 1000
flood_protect_remip 10000
flood_protect_newmap 10000

global enable_teamkill_limiter 1
global teamkill_limit 7
global teamkill_bantime (mins 30)
global teamkill_show_public 0
global teamkill_showlimit 1

global enable_ping_limiter 1
global ping_limiter_tick 25000
global ping_limit 500
global lag_limit 30
global ping_limit_warnings 4

global enable_dynamic_maxclients 0

global disable_masterauth_in_coopedit 0

global shell_label "server"

global use_kickspec 0
global kickspec_maxtime (mins 25)

global enable_ownage_messages 0

global enable_suddendeath 0

global change_default_maptime 0
global default_maptime (mins 15)

global use_spec_inactives 0
global spec_inactives_check_time (mins 6)
global spec_inactives_time (mins 5)

global use_cd_modmap 0

global use_cd_accuracy 0
global cd_accuracy_limit 0

global use_cd_chainsawhack 0

global resize_totalminplayers 1
global resize_totalmaxplayers 7

global use_resize_mastermode 0
global resize_mastermode locked
global resize_totalmaxplayers 50

global cheater_ad_timer (mins 4)
global votekick_ad_timer (mins 4)

global master_domains []

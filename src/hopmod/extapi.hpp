#ifndef HOPMOD_EXTAPI_HPP
#define HOPMOD_EXTAPI_HPP

#include "cube.h"
#include "lua.hpp"
#include "utils.hpp"
#include <string>
#include <vector>
#include <iostream>

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

namespace server
{
    namespace aiman
    {
        extern int botlimit;
        extern bool botbalance;
        extern bool deleteai();
    }
    
    extern string serverdesc;
    extern string smapname;
    extern string serverpass;
    extern string masterpass;
    extern int currentmaster;
    extern int minremain;
    extern bool reassignteams;
    extern int gamemillis;
    extern int gamelimit;
    
    extern int mastermode;
    extern int mastermode_owner;
    extern string next_gamemode;
    extern string next_mapname;
    extern int next_gametime;
    
    extern int reservedslots;
    
    extern bool allow_mm_veto;
    extern bool allow_mm_locked;
    extern bool allow_mm_private;
    extern bool allow_item[11];
    extern bool allow_master;
    extern string authserver_hostname;
    extern bool broadcast_mapmodified;
    extern timer::time_diff_t timer_alarm_threshold;
    
    extern bool enable_extinfo;
    extern bool kick_bannedip_group;
    
    void init_hopmod();
    void reload_hopmod();
    void update_hopmod();
    
    int player_sessionid(int);
    int player_id(int);
    void player_msg(int,const char *);
    const char * player_name(int);
    const char * player_team(int);
    const char * player_privilege(int);
    int player_privilege_code(int);
    int player_ping(int);
    int player_lag(int);
    const char * player_ip(int);
    int player_iplong(int);
    const char * player_status(int);
    int player_status_code(int);
    int player_frags(int);
    int player_deaths(int);
    int player_suicides(int);
    int player_teamkills(int);
    int player_damage(int);
    int player_damagewasted(int);
    int player_maxhealth(int);
    int player_health(int);
    int player_gun(int);
    int player_hits(int);
    int player_shots(int);
    int player_accuracy(int);
    int player_connection_time(int);
    int player_timeplayed(int);
    int player_win(int);
    void player_spec(int);
    void player_unspec(int);
    int player_bots(int);
    int player_pos(lua_State *);
    std::vector<float> player_pos(int);
    void unsetmaster();
    bool server_setmaster(int);
    void server_setadmin(int);
    void player_slay(int);
    bool player_changeteam(int,const char *);
    int player_authreq(int);
    int player_rank(int);
    bool player_isbot(int);
    void set_invadmin(int);
    void unset_invadmin(int);
    int player_mapcrc(int);
    
    void team_msg(const char *,const char *);
    std::vector<std::string> get_teams();
    int lua_team_list(lua_State * L);
    int get_team_score(const char * );
    std::vector<int> get_team_players(const char * team);
    int lua_team_players(lua_State *);
    int team_win(const char *);
    int team_draw(const char *);
    
    void pausegame(bool);
    void kick(int cn,int time,const std::string & admin,const std::string & reason);
    void changetime(int remaining);
    void clearbans();
    void changemap(const char * map,const char * mode,int mins);
    int modecode(const char * modename);
    int getplayercount();
    int getbotcount();
    int getspeccount();
    bool writebanlist(const char * filename);
    bool loadbanlist(const char * filename);
    void addpermban(const char * addr);
    void unsetban(const char * addr);
    int addbot(int);
    int delbot();
    void enable_master_auth(bool);
    bool using_master_auth();
    void update_mastermask();
    const char * gamemodename();
    int lua_gamemodeinfo(lua_State *);
    int recorddemo(const char *);
    void enddemorecord();
    void calc_player_ranks();
    void script_set_mastermode(int);
    void add_allowed_ip(const char *);
    
    std::vector<int> cs_player_list();
    std::vector<int> cs_spec_list();
    std::vector<int> cs_bot_list();
    int lua_player_list(lua_State *);
    int lua_spec_list(lua_State *);
    int lua_bot_list(lua_State *);

    bool selectnextgame();
    
    int lua_genkeypair(lua_State *);
    int lua_genchallenge(lua_State *);
    int lua_checkchallenge(lua_State *);
    int lua_freechalanswer(lua_State *);
    
    bool delegateauth(int);
    bool relayauthanswer(int,const char *);
    void sendauthchallenge(int,const char *);
    void sendauthreq(int, const char *);
    void signal_auth_success(int,int);
    void signal_auth_failure(int,int);
    
    void authfailed(uint id);
    void authsucceeded(uint id);
    void authchallenged(uint id, const char *val);
    
    void restart_now();
    
    void suicide(int);
    
} //namespace server

#endif

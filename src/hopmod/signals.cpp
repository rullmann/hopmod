#ifdef BOOST_BUILD_PCH_ENABLED
#include "pch.hpp"
#endif

#include "hopmod.hpp"

#include <fungu/script.hpp>
#include <fungu/script/slot_factory.hpp>
#include <fungu/script/lua/lua_function.hpp>
using namespace fungu;

static script::env * env = NULL;
static script::slot_factory slots;

boost::signal<void ()> signal_started;
boost::signal<void (int)> signal_shutdown;
boost::signal<void ()> signal_shutdown_scripting;
boost::signal<void ()> signal_reloadhopmod;

boost::signal<void (int)> signal_connect;
boost::signal<void (int,const char *)> signal_disconnect;
boost::signal<void (const char *,const char *)> signal_failedconnect;
boost::signal<void (int)> signal_active;
boost::signal<void (int,const char *,const char *)> signal_rename;
boost::signal<void (int)> signal_renaming;
boost::signal<void (int,const char *,const char *)> signal_reteam;
boost::signal<int (int,const char *,const char *), proceed> signal_chteamrequest;
boost::signal<void (int,int,std::string,std::string)> signal_kick;
boost::signal<int (int,const char *), proceed> signal_text;
boost::signal<int (int,const char *), proceed> signal_sayteam;
boost::signal<void ()> signal_intermission;
boost::signal<void ()> signal_finishedgame;
boost::signal<int (int),maxvalue> signal_timeupdate;
boost::signal<void (const char *,const char *)> signal_mapchange;
boost::signal<int (int,const char *,const char *), proceed> signal_mapvote;
boost::signal<void ()> signal_setnextgame;
boost::signal<void ()> signal_gamepaused;
boost::signal<void ()> signal_gameresumed;
boost::signal<int (int, const char *,const char *),proceed> signal_setmastermode;
boost::signal<void (int,int)> signal_spectator;
boost::signal<void (int,int,int)> signal_privilege;
boost::signal<void (int,int)> signal_teamkill;
boost::signal<void (int,int)> signal_frag;
boost::signal<void (int,const char *,const char *)> signal_authreq;
boost::signal<void (int,int,const char *)> signal_authrep;
boost::signal<void (int,int,int)> signal_addbot;
boost::signal<void (int)> signal_delbot;
boost::signal<void (int)> signal_botleft;
boost::signal<void (int,const char *)> signal_beginrecord;
boost::signal<void (int,int)> signal_endrecord;
boost::signal<void (int)> signal_mapcrcfail;
boost::signal<void (const char *,const char *)> signal_votepassed;
boost::signal<void (int,int,int)> signal_shot;
boost::signal<void (int)> signal_suicide;
boost::signal<void (int)> signal_takeflag;
boost::signal<void (int)> signal_dropflag;
boost::signal<void (int)> signal_scoreflag;
boost::signal<void (int)> signal_returnflag;
boost::signal<void ()> signal_maintenance;
boost::signal<void (int)> signal_spawn;

static void destroy_slot(int handle);
static int lua_event_handler(lua_State * L);

class event_handler_object
{
    static const char * MT;
public:
    event_handler_object()
    :handler_id(-1)
    {
        
    }
    
    static int register_class(lua_State * L)
    {
        luaL_newmetatable(L, MT);
        lua_pushvalue(L, -1);
        
        static luaL_Reg funcs[] = {
            {"__gc", &event_handler_object::__gc},
            {"cancel", &event_handler_object::cancel},
            {NULL, NULL}
        };
        
        luaL_register(L, NULL, funcs);
        
        lua_setfield(L, -1, "__index");
        
        return 0;
    }
    
    static int create(lua_State * L)
    {
        event_handler_object * self = new (lua_newuserdata(L, sizeof(event_handler_object))) event_handler_object();
        luaL_getmetatable(L, MT);
        lua_setmetatable(L, -2);
        
        lua_event_handler(L);
        self->handler_id = lua_tointeger(L, -1);
        lua_pushvalue(L, -2); //FIXME assert -2 is the event handler object
        return 1;
    }
    
    static int __gc(lua_State * L)
    {
        cancel(L);
        return 0;
    }
    
    static int cancel(lua_State * L)
    {
        event_handler_object * self = reinterpret_cast<event_handler_object *>(luaL_checkudata(L, 1, MT));
        destroy_slot(self->handler_id);
        return 0;
    }
private:
    int handler_id;
};

const char * event_handler_object::MT = "event_handler_object";

static script::any proceed_error_handler(script::error_trace * errinfo)
{
    report_script_error(errinfo);
    return true;
}

static script::any normal_error_handler(script::error_trace * errinfo)
{
    report_script_error(errinfo);
    return script::any::null_value();
}

/*
    Register function as an event handler
*/
static int lua_event_handler(lua_State * L)
{
    const char * name = luaL_checkstring(L, 1);
    
    luaL_checktype(L, 2, LUA_TFUNCTION);
    lua_pushvalue(L, 2);
    
    script::env_object::shared_ptr luaFunctionObject = new script::lua::lua_function(L);
    luaFunctionObject->set_adopted();

    int handle = slots.create_slot(name, luaFunctionObject, env);
    lua_pushinteger(L, handle);
    
    return 1;
}

/*
    Register function as an event handler
*/
static int cubescript_event_handler(const std::string & name, script::any obj)
{
    if(obj.get_type() != typeid(script::env_object::shared_ptr)) throw script::error(script::BAD_CAST);
    return slots.create_slot(name, script::any_cast<script::env_object::shared_ptr>(obj), env);
}

/*
    Cancel event handler
*/
static void destroy_slot(int handle)
{
    slots.destroy_slot(handle);
}

static void cleanup(int)
{
    slots.clear();
    slots.deallocate_destroyed_slots();
}

void register_signals(script::env & env)
{
    ::env = &env;
    
    signal_shutdown.connect(&cleanup, boost::signals::at_front);
    
    slots.register_signal(signal_started, "started", normal_error_handler);
    slots.register_signal(signal_shutdown,"shutdown",normal_error_handler, boost::signals::at_front);
    slots.register_signal(signal_shutdown_scripting, "shutdown_scripting", normal_error_handler);
    slots.register_signal(signal_reloadhopmod, "reloadhopmod", normal_error_handler);
    slots.register_signal(signal_maintenance, "maintenance", normal_error_handler);
    
    slots.register_signal(signal_connect,"connect",normal_error_handler);
    slots.register_signal(signal_disconnect,"disconnect",normal_error_handler);
    slots.register_signal(signal_failedconnect, "failedconnect",normal_error_handler);
    slots.register_signal(signal_active, "active", normal_error_handler);
    slots.register_signal(signal_rename,"rename",normal_error_handler);
    slots.register_signal(signal_renaming, "renaming", normal_error_handler);
    slots.register_signal(signal_reteam, "reteam", normal_error_handler);
    slots.register_signal(signal_chteamrequest, "chteamrequest", proceed_error_handler);
    slots.register_signal(signal_kick,"kick",normal_error_handler);
    slots.register_signal(signal_text,"text",proceed_error_handler);
    slots.register_signal(signal_sayteam,"sayteam",proceed_error_handler);
    slots.register_signal(signal_intermission,"intermission", normal_error_handler);
    slots.register_signal(signal_finishedgame, "finishedgame", normal_error_handler);
    slots.register_signal(signal_timeupdate,"timeupdate", normal_error_handler);
    slots.register_signal(signal_mapchange,"mapchange", normal_error_handler);
    slots.register_signal(signal_mapvote, "mapvote", proceed_error_handler);
    slots.register_signal(signal_setnextgame, "setnextgame", normal_error_handler);
    slots.register_signal(signal_gamepaused, "gamepaused", normal_error_handler);
    slots.register_signal(signal_gameresumed, "gameresumed", normal_error_handler);
    slots.register_signal(signal_setmastermode, "setmastermode", proceed_error_handler);
    slots.register_signal(signal_spectator, "spectator", normal_error_handler);
    slots.register_signal(signal_privilege, "privilege", normal_error_handler);
    slots.register_signal(signal_teamkill, "teamkill", normal_error_handler);
    slots.register_signal(signal_frag, "frag", normal_error_handler);
    slots.register_signal(signal_authreq, "request_auth_challenge", normal_error_handler);
    slots.register_signal(signal_authrep, "auth_challenge_response", normal_error_handler);
    slots.register_signal(signal_addbot, "addbot", normal_error_handler);
    slots.register_signal(signal_delbot, "delbot", normal_error_handler);
    slots.register_signal(signal_botleft, "botleft", normal_error_handler);
    slots.register_signal(signal_beginrecord, "beginrecord", normal_error_handler);
    slots.register_signal(signal_endrecord, "endrecord", normal_error_handler);
    slots.register_signal(signal_mapcrcfail, "mapcrcfail", normal_error_handler);
    slots.register_signal(signal_votepassed, "votepassed", normal_error_handler);
    slots.register_signal(signal_shot, "shot", normal_error_handler);
    slots.register_signal(signal_suicide, "suicide", normal_error_handler);
    slots.register_signal(signal_takeflag, "takeflag", normal_error_handler);
    slots.register_signal(signal_dropflag, "dropflag", normal_error_handler);
    slots.register_signal(signal_scoreflag, "scoreflag", normal_error_handler);
    slots.register_signal(signal_returnflag, "returnflag", normal_error_handler);
    slots.register_signal(signal_spawn, "spawn", normal_error_handler);
    
    script::bind_freefunc(cubescript_event_handler, "event_handler", env);
    script::bind_freefunc(destroy_slot, "cancel_handler", env);
    
    register_lua_function(lua_event_handler,"event_handler");
    
    event_handler_object::register_class(env.get_lua_state());
    register_lua_function(event_handler_object::create, "event_handler_object");
}

void cleanup_dead_slots()
{
    slots.deallocate_destroyed_slots();
}

void disconnect_all_slots()
{
    signal_started.disconnect_all_slots();
    signal_shutdown.disconnect_all_slots();
    signal_shutdown_scripting.disconnect_all_slots();
    signal_reloadhopmod.disconnect_all_slots();
    signal_maintenance.disconnect_all_slots();
    
    signal_connect.disconnect_all_slots();
    signal_disconnect.disconnect_all_slots();
    signal_failedconnect.disconnect_all_slots();
    signal_active.disconnect_all_slots();
    signal_rename.disconnect_all_slots();
    signal_renaming.disconnect_all_slots();
    signal_reteam.disconnect_all_slots();
    signal_chteamrequest.disconnect_all_slots();
    signal_kick.disconnect_all_slots();
    signal_text.disconnect_all_slots();
    signal_sayteam.disconnect_all_slots();
    signal_intermission.disconnect_all_slots();
    signal_finishedgame.disconnect_all_slots();
    signal_timeupdate.disconnect_all_slots();
    signal_mapchange.disconnect_all_slots();
    signal_mapvote.disconnect_all_slots();
    signal_setnextgame.disconnect_all_slots();
    signal_gamepaused.disconnect_all_slots();
    signal_gameresumed.disconnect_all_slots();
    signal_setmastermode.disconnect_all_slots();
    signal_spectator.disconnect_all_slots();
    signal_privilege.disconnect_all_slots();
    signal_teamkill.disconnect_all_slots();
    signal_frag.disconnect_all_slots();
    signal_authreq.disconnect_all_slots();
    signal_authrep.disconnect_all_slots();
    signal_addbot.disconnect_all_slots();
    signal_delbot.disconnect_all_slots();
    signal_botleft.disconnect_all_slots();
    signal_beginrecord.disconnect_all_slots();
    signal_endrecord.disconnect_all_slots();
    signal_mapcrcfail.disconnect_all_slots();
    signal_votepassed.disconnect_all_slots();
    signal_shot.disconnect_all_slots();
    signal_suicide.disconnect_all_slots();
    signal_takeflag.disconnect_all_slots();
    signal_dropflag.disconnect_all_slots();
    signal_scoreflag.disconnect_all_slots();
    signal_returnflag.disconnect_all_slots();
    signal_spawn.disconnect_all_slots();
}


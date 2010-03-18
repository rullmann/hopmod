#ifdef BOOST_BUILD_PCH_ENABLED
#include "pch.hpp"
#endif

#include "hopmod.hpp"
#include "handle_resolver.hpp"

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

boost::signal<int (int, const char *, const char *, const char *), proceed> signal_connecting;
boost::signal<void (int)> signal_connect;
boost::signal<void (int,const char *)> signal_disconnect;
boost::signal<void (const char *,const char *)> signal_failedconnect;
boost::signal<void (int)> signal_maploaded;
boost::signal<void (int,const char *,const char *)> signal_rename;
boost::signal<void (int,int)> signal_renaming;
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
boost::signal<int (int,int,int,int), proceed> signal_damage;
boost::signal<int (int,int), proceed> signal_pong;
boost::signal<int (int,const char*), proceed> signal_setmaster;

static void destroy_slot(int handle);
namespace lua{
static int register_event_handler(lua_State * L);
static void cleanup(lua_State * L);
}//namespace lua

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
        
        lua::register_event_handler(L);
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

namespace lua{

typedef std::vector<int> lua_function_vector;
static std::map<std::string, lua_function_vector> created_event_slots;
typedef std::map<std::string, lua_function_vector>::iterator created_event_slots_iterator;
typedef std::map<int, std::pair<lua_function_vector *, int> > handle_slot_map;
static handle_slot_map handle_to_slot;
static handle_resolver<created_event_slots_iterator> signal_handles;

static lua_function_vector::iterator get_free_vector_iterator(lua_function_vector & v)
{
    for(lua_function_vector::iterator it = v.begin(); it != v.end(); it++)
        if(*it == -1) return it;
    v.push_back(-1);
    return v.end() - 1;
}

static int event_trigger(lua_State * L)
{
    lua_function_vector & handlers = *reinterpret_cast<lua_function_vector *>(lua_touserdata(L, lua_upvalueindex(1)));
    int argc = lua_gettop(L);
    
    for(lua_function_vector::const_iterator it = handlers.begin(); it != handlers.end(); it++)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, *it);
        for(int i = 1; i <= argc; i++) 
            lua_pushvalue(L, i);
        if(lua_pcall(L, argc, 0, 0) != 0)
        {
            report_script_error(lua_tostring(L, -1));
        }
    }
    
    return 0;
}

static int create_signal(lua_State * L)
{
    const char * name = luaL_checkstring(L, 1);
    
    std::pair<created_event_slots_iterator, bool> inserted = created_event_slots.insert(std::pair<std::string, lua_function_vector>(name, std::vector<int>()));
    
    if(inserted.second == false)
    {
        luaL_argerror(L, 1, "name already in use");
        return 0;
    }
    
    lua_pushlightuserdata(L, &inserted.first->second);
    lua_pushcclosure(L, &event_trigger, 1);
    
    lua_pushinteger(L, signal_handles.assign(inserted.first));
    
    return 2;
}

static int register_event_handler(lua_State * L)
{
    const char * name = luaL_checkstring(L, 1);
    
    luaL_checktype(L, 2, LUA_TFUNCTION);
    lua_pushvalue(L, 2);
    
    script::env_object::shared_ptr luaFunctionObject = new script::lua::lua_function(L);
    luaFunctionObject->set_adopted();

    int handle = slots.create_slot(name, luaFunctionObject, env);
    
    if(handle == -1)
    {
        std::map<std::string, lua_function_vector>::iterator it = created_event_slots.find(name);
        if(it != created_event_slots.end())
        {
            lua_function_vector::iterator pos = get_free_vector_iterator(it->second);
            
            lua_pushvalue(L, 2);
            *pos = luaL_ref(L, LUA_REGISTRYINDEX);
            
            handle = slots.skip_slot_id();
            handle_to_slot[handle] = std::pair<lua_function_vector *, int>(&it->second, static_cast<int>(pos - it->second.begin()));
        }
    }
    
    lua_pushinteger(L, handle);
    
    return 1;
}

static int cancel_signal(lua_State * L)
{
    int signalId = luaL_checkint(L, 1);
    created_event_slots_iterator iter = signal_handles.resolve(signalId);
    if(iter == created_event_slots_iterator()) return 0;
    
    for(lua_function_vector::const_iterator functionIter = iter->second.begin(); functionIter != iter->second.end(); ++functionIter)
        luaL_unref(L, LUA_REGISTRYINDEX, *functionIter);
    
    created_event_slots.erase(iter);
    signal_handles.free(signalId);
    
    return 0;
}

static void cancel_event_handler(lua_State * L, int handle)
{
    handle_slot_map::iterator it = handle_to_slot.find(handle);
    if(it == handle_to_slot.end()) return;
    int index = it->second.second;
    int luaFunctionRef = (*it->second.first)[index];
    luaL_unref(L, LUA_REGISTRYINDEX, luaFunctionRef);
    (*it->second.first)[index] = -1;
}

static int destroy_slot(lua_State * L)
{
    int handle = luaL_checkint(L, 1);
    if(handle < 0) return 0;
    bool done = slots.destroy_slot(handle);
    if(!done) cancel_event_handler(L, handle);
    return 0;
}

static void cleanup(lua_State * L)
{
    for(handle_slot_map::iterator it = handle_to_slot.begin(); it != handle_to_slot.end(); it++)
        luaL_unref(L, LUA_REGISTRYINDEX, (*it->second.first)[it->second.second]);
    handle_to_slot.clear();
    created_event_slots.clear();
}

} //namespace lua

namespace cubescript{    
static int register_event_handler(const std::string & name, script::any obj)
{
    if(obj.get_type() != typeid(script::env_object::shared_ptr)) throw script::error(script::BAD_CAST);
    return slots.create_slot(name, script::any_cast<script::env_object::shared_ptr>(obj), env);
}
}//namespace cubescript

/*
    Cancel event handler
*/
static void destroy_slot(int handle)
{
    if(handle < 0) return;
    slots.destroy_slot(handle);
}

static void cleanup(int)
{
    slots.clear();
    slots.deallocate_destroyed_slots();
    lua::cleanup(env->get_lua_state());
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
    
    slots.register_signal(signal_connecting, "connecting", normal_error_handler);
    slots.register_signal(signal_connect,"connect",normal_error_handler);
    slots.register_signal(signal_disconnect,"disconnect",normal_error_handler);
    slots.register_signal(signal_failedconnect, "failedconnect",normal_error_handler);
    slots.register_signal(signal_maploaded, "maploaded", normal_error_handler);
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
    slots.register_signal(signal_damage, "damage", normal_error_handler);
    slots.register_signal(signal_pong, "pong", normal_error_handler);
    slots.register_signal(signal_setmaster, "setmaster", normal_error_handler);
    
    
    script::bind_freefunc(cubescript::register_event_handler, "event_handler", env);
    script::bind_freefunc(destroy_slot, "cancel_handler", env);
    
    register_lua_function(lua::register_event_handler,"event_handler");
    register_lua_function(lua::destroy_slot, "cancel_handler");
    register_lua_function(lua::create_signal, "create_event_signal");
    register_lua_function(lua::cancel_signal, "cancel_event_signal");
    
    event_handler_object::register_class(env.get_lua_state());
    register_lua_function(event_handler_object::create, "event_handler_object"); //deprecated
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
    
    signal_connecting.disconnect_all_slots();
    signal_connect.disconnect_all_slots();
    signal_disconnect.disconnect_all_slots();
    signal_failedconnect.disconnect_all_slots();
    signal_maploaded.disconnect_all_slots();
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
    signal_damage.disconnect_all_slots();
}


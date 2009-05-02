#ifndef HOPMOD_SIGNALS_HPP
#define HOPMOD_SIGNALS_HPP

#include "env_fwd.hpp"
#include <boost/signal.hpp>

/**
    @brief Event proceed combiner
    
    If any slot function returns -1 it will tell the signaller to veto/cancel the event.
*/
struct proceed
{
    typedef int result_type;
    static const int true_value = 0;
    template<typename InputIterator>
    int operator()(InputIterator first, InputIterator last)const
    {
        for(InputIterator it = first; it != last; ++it)
            if(*it == -1) return -1;
        return 0;
    }
};

// Player Events
extern boost::signal<void (int)>                                    signal_connect;
extern boost::signal<void (int,const char *)>                       signal_disconnect;
extern boost::signal<void (const char *,const char *)>              signal_failedconnect;
extern boost::signal<void (int)>                                    signal_active;
extern boost::signal<void (int)>                                    signal_renaming;
extern boost::signal<void (int,const char *,const char *)>          signal_rename;
extern boost::signal<void (int,const char *,const char *)>          signal_reteam;
extern boost::signal<int (int,const char *,const char *),proceed>   signal_chteamrequest;
extern boost::signal<void (int,int,std::string,std::string)>        signal_kick;
extern boost::signal<int (int,const char *), proceed>               signal_text;
extern boost::signal<int (int,const char *), proceed>               signal_sayteam;
extern boost::signal<int (int,const char *,const char *), proceed>  signal_mapvote;
extern boost::signal<void (const char *,const char *)>              signal_setmastermode;
extern boost::signal<void (int,int)>                                signal_spectator;
extern boost::signal<void (int,const char *,int)>                   signal_setmaster;
extern boost::signal<void (int,int)>                                signal_teamkill;
extern boost::signal<void (int,const char *,const char *,bool)>     signal_auth;
extern boost::signal<void (int,const char *,const char *)>          signal_authreq;
extern boost::signal<void (int,const char *)>                       signal_authrep;
extern boost::signal<void (int,int,int)>                            signal_addbot;
extern boost::signal<void (int)>                                    signal_delbot;
extern boost::signal<void (int)>                                    signal_botleft;
extern boost::signal<void (int)>                                    signal_mapcrcfail;
extern boost::signal<void (int,int)>                                signal_teamkill;

// Game Events
extern boost::signal<void ()>                                signal_intermission;
extern boost::signal<void ()>                                signal_finishedgame;
extern boost::signal<void (int)>                             signal_timeupdate;
extern boost::signal<void (const char *,const char *)>       signal_mapchange;
extern boost::signal<void ()>                                signal_setnextgame;
extern boost::signal<void ()>                                signal_gamepaused;
extern boost::signal<void ()>                                signal_gameresumed;
extern boost::signal<void (int,const char *)>                signal_beginrecord;
extern boost::signal<void (int,int)>                         signal_endrecord;

// Server Events
extern boost::signal<void ()> signal_started;
extern boost::signal<void ()> signal_shutdown;
extern boost::signal<void (const char *,const char *)>      signal_votepassed;
extern boost::signal<void ()> signal_reloadhopmod;

/**
    @brief Register signals with the global script::slot_factory instance.
*/
void register_signals(fungu::script::env &);

/**
    @brief Deallocates destroyed slots - should be called regularly on the main loop.
*/
void cleanup_dead_slots();

void disconnect_all_slots();

#endif

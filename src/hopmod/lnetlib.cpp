/*
    A Lua Network Library module developed for Hopmod
    Copyright (C) 2009 Graham Daws
*/
#include "lnetlib.hpp"
#include "main_io_service.hpp"
#include "hopmod.hpp"
#include <netinet/in.h> //byte ordering functions
#include <unistd.h> //unlink()
#include <iostream>
#include <boost/asio.hpp>
using namespace boost::asio;
#include "buffered_socket.hpp"

typedef buffered_input_socket<ip::tcp::socket> buffered_tcp_socket;
typedef buffered_input_socket<local::stream_protocol::socket> buffered_local_socket;

struct lnetlib_buffer
{
    char * start;
    char * end;
    char * produced;
    char * consumed;
    
    size_t get_read_left()const
    {
        return produced - consumed;
    }
    
    size_t get_write_left()const
    {
        return end - produced;
    }
    
    size_t get_size()const
    {
        return end - start;
    }
};

static const char * TCP_ACCEPTOR_MT = "lnetlib_tcp_acceptor";
static const char * BASIC_TCP_SOCKET_MT = "lnetlib_basic_tcp_socket";
static const char * TCP_CLIENT_SOCKET_MT = "lnetlib_tcp_client_socket";
static const char * BUFFER_MT = "lnetlib_buffer";
static const char * UDP_SOCKET_MT = "lnetlib_udp_socket";
static const char * LOCAL_SOCKET_MT = "lnetlib_local_socket";
static const char * LOCAL_ACCEPTOR_MT = "lnetlib_local_acceptor";
static const char * LOCAL_SOCKET_CLIENT_MT = "lnetlib_local_client";

static lua_State * lua_state;
static io_service * main_io;

void * lua_aux_checkobject(lua_State * L, int narg, const char * tname)
{
    luaL_checktype(L, narg, LUA_TUSERDATA);
    luaL_getmetatable(L, tname);
    lua_getmetatable(L, narg);
    
    while(1)
    {
        if(lua_equal(L, -1, -2)) return lua_touserdata(L, narg);
        else
        {
            lua_getmetatable(L, -1);
            if(lua_type(L, -1) == LUA_TNIL) break;
            lua_replace(L, -2);
        }
    }
    
    luaL_argerror(L, narg, "incompatible type");
    return NULL;
}

template<typename EndpointType>
void push_endpoint(lua_State * L, const EndpointType & endpoint)
{
    lua_newtable(L);
    
    lua_pushinteger(L, endpoint.port());
    lua_setfield(L, -2, "port");
    
    lua_pushstring(L, endpoint.address().to_string().c_str());
    lua_setfield(L, -2, "ip");
    
    if(endpoint.address().is_v4())
    {
        lua_pushinteger(L, endpoint.address().to_v4().to_ulong());
        lua_setfield(L, -2, "iplong");
    }
}

void resolve_handler(int luaFunctionCbRef, const boost::system::error_code ec, ip::tcp::resolver::iterator it)
{
    lua_rawgeti(lua_state, LUA_REGISTRYINDEX, luaFunctionCbRef);
    luaL_unref(lua_state, LUA_REGISTRYINDEX, luaFunctionCbRef);
    
    if(!ec)
    {
        lua_newtable(lua_state);
        
        int count = 1;
        for(; it != ip::tcp::resolver::iterator(); ++it)
        {
            lua_pushinteger(lua_state, count++);
            lua_pushstring(lua_state, it->endpoint().address().to_string().c_str());
            lua_settable(lua_state, -3);
        }
        
        if(lua_pcall(lua_state, 1, 0, 0) != 0)
            report_script_error(lua_tostring(lua_state, -1));
    }
    else
    {
        lua_pushstring(lua_state, ec.message().c_str());
        
        if(lua_pcall(lua_state, 1, 0, 0) != 0)
            report_script_error(lua_tostring(lua_state, -1));
    }
}

int async_resolve(lua_State * L)
{
    static ip::tcp::resolver dns(*main_io);
    const char * hostname = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    lua_pushvalue(L, 2);
    ip::tcp::resolver::query query(hostname, "");
    dns.async_resolve(query, boost::bind(resolve_handler, luaL_ref(L, LUA_REGISTRYINDEX), _1, _2));
    return 0;
}

int acceptor_listen(lua_State * L)
{
    ip::tcp::acceptor * acceptor = reinterpret_cast<ip::tcp::acceptor *>(luaL_checkudata(L, 1, TCP_ACCEPTOR_MT));
    acceptor->listen();
    return 0;
}

int acceptor_close(lua_State * L)
{
    ip::tcp::acceptor * acceptor = reinterpret_cast<ip::tcp::acceptor *>(luaL_checkudata(L, 1, TCP_ACCEPTOR_MT));
    acceptor->close();
    return 0;
}

void async_accept_handler(int luaFunctionCbRef, int socketRef, boost::system::error_code error)
{
    lua_rawgeti(lua_state, LUA_REGISTRYINDEX, luaFunctionCbRef);
    luaL_unref(lua_state, LUA_REGISTRYINDEX, luaFunctionCbRef);
    
    lua_rawgeti(lua_state, LUA_REGISTRYINDEX, socketRef);
    luaL_unref(lua_state, LUA_REGISTRYINDEX, socketRef);
    
    if(error)
    {
        lua_pop(lua_state, 1); //socket object
        lua_pushstring(lua_state, error.message().c_str());
        
        if(lua_pcall(lua_state, 2, 0, 0) != 0)
            report_script_error(lua_tostring(lua_state, -1));
    }
    else
    {
        if(lua_pcall(lua_state, 1, 0, 0) != 0)
            report_script_error(lua_tostring(lua_state, -1));
    }
}

int acceptor_async_accept(lua_State * L)
{
    luaL_checktype(L, 2, LUA_TFUNCTION);
    ip::tcp::acceptor * acceptor = reinterpret_cast<ip::tcp::acceptor *>(luaL_checkudata(L, 1, TCP_ACCEPTOR_MT));
    
    buffered_tcp_socket * socket = new (lua_newuserdata(L, sizeof(buffered_tcp_socket))) buffered_tcp_socket(*main_io);
    luaL_getmetatable(L, BASIC_TCP_SOCKET_MT);
    lua_setmetatable(L, -2);
    int socketRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    lua_pushvalue(L, 2);
    acceptor->async_accept(*socket, boost::bind(async_accept_handler, luaL_ref(L, LUA_REGISTRYINDEX), socketRef, _1));
    
    return 0;
}

int acceptor_gc(lua_State * L)
{
    reinterpret_cast<ip::tcp::acceptor *>(luaL_checkudata(L, 1, TCP_ACCEPTOR_MT))->~basic_socket_acceptor<ip::tcp>();
    return 0;
}

int create_tcp_acceptor(lua_State * L)
{
    const char * ip = luaL_checkstring(L, 1);
    int port = luaL_checkint(L, 2);
    
    ip::tcp::acceptor * acceptor = new (lua_newuserdata(L, sizeof(ip::tcp::acceptor))) ip::tcp::acceptor(*main_io);
    
    // Setup metatable with __gc function set before bind() so the user-data object will be destroyed if the bind op fails.
    luaL_getmetatable(L, TCP_ACCEPTOR_MT);
    lua_setmetatable(L, -2);
    
    ip::tcp::endpoint server_ep(ip::address_v4::from_string(ip), port);
    
    acceptor->open(server_ep.protocol());
    acceptor->set_option(ip::tcp::acceptor::reuse_address(true));
    
    boost::system::error_code error;
    acceptor->bind(server_ep, error);
    if(error)
    {
        lua_pushnil(L);
        lua_pushstring(L, error.message().c_str());
        return 2;
    }
    
    // Return user-data object
    return 1;
}

int acceptor_set_option(lua_State * L)
{
    ip::tcp::acceptor * acceptor = reinterpret_cast<ip::tcp::acceptor *>(luaL_checkudata(L, 1, TCP_ACCEPTOR_MT));
    const char * optname = luaL_checkstring(L, 2);
    
    if(!strcmp(optname, "enable_connection_aborted"))
    {
        boost::asio::socket_base::enable_connection_aborted option(luaL_checkint(L, 3));
        acceptor->set_option(option);
    }
    else if(!strcmp(optname, "reuse_address"))
    {
        boost::asio::socket_base::reuse_address option(luaL_checkint(L, 3));
        acceptor->set_option(option);
    }
    
    return 0;
}

int acceptor_get_option(lua_State * L)
{
    ip::tcp::acceptor * acceptor = reinterpret_cast<ip::tcp::acceptor *>(luaL_checkudata(L, 1, TCP_ACCEPTOR_MT));
    const char * optname = luaL_checkstring(L, 2);
    
    if(!strcmp(optname, "enable_connection_aborted"))
    {
        boost::asio::socket_base::enable_connection_aborted option;
        acceptor->get_option(option);
        lua_pushboolean(L, option.value());
        return 1;
    }
    else if(!strcmp(optname, "reuse_address"))
    {
        boost::asio::socket_base::reuse_address option;
        acceptor->get_option(option);
        lua_pushboolean(L, option.value());
        return 1;
    }
    
    return 0;
}

void create_acceptor_metatable(lua_State * L)
{
    luaL_newmetatable(L, TCP_ACCEPTOR_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    
    static luaL_Reg funcs[] = {
        {"__gc", acceptor_gc},
        {"listen", acceptor_listen},
        {"close", acceptor_close},
        {"async_accept", acceptor_async_accept},
        {"set_option", acceptor_set_option},
        {"get_option", acceptor_get_option},
        {NULL, NULL}
    };
    
    luaL_register(L, NULL, funcs);
}

int socket_gc(lua_State * L)
{
    reinterpret_cast<buffered_tcp_socket *>(lua_aux_checkobject(L, 1, BASIC_TCP_SOCKET_MT))->~buffered_tcp_socket();
    return 0;
}

int socket_close(lua_State * L)
{
    reinterpret_cast<buffered_tcp_socket *>(lua_aux_checkobject(L, 1, BASIC_TCP_SOCKET_MT))->close();
    return 0;
}

void async_send_handler(int functionRef, int stringRef, const boost::system::error_code error, const size_t len)
{
    luaL_unref(lua_state, LUA_REGISTRYINDEX, stringRef);
    
    lua_rawgeti(lua_state, LUA_REGISTRYINDEX, functionRef);
    luaL_unref(lua_state, LUA_REGISTRYINDEX, functionRef);

    if(error) lua_pushstring(lua_state, error.message().c_str());
    else lua_pushnil(lua_state);
    
    lua_pushinteger(lua_state, len);
    
    if(lua_pcall(lua_state, 2, 0, 0) != 0)
        report_script_error(lua_tostring(lua_state, -1));
}

int socket_async_send_buffer(lua_State *);

int socket_async_send(lua_State * L)
{
    buffered_tcp_socket * socket = reinterpret_cast<buffered_tcp_socket *>(lua_aux_checkobject(L, 1, BASIC_TCP_SOCKET_MT));
    
    if(lua_type(L, 2) != LUA_TSTRING) return socket_async_send_buffer(L);

    luaL_checktype(L, 3, LUA_TFUNCTION);
    
    size_t stringlen;
    const char * str = lua_tolstring(L, 2, &stringlen);
    lua_pushvalue(L, -1);
    int stringRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    lua_pushvalue(L, 3);
    socket->async_send(boost::asio::buffer(str, stringlen), boost::bind(async_send_handler, luaL_ref(L, LUA_REGISTRYINDEX), stringRef, _1, _2));
    
    return 0;
}

void async_read_until_handler(int functionRef, boost::asio::streambuf * buf, const boost::system::error_code error, const size_t readlen)
{    
    lua_rawgeti(lua_state, LUA_REGISTRYINDEX, functionRef);
    luaL_unref(lua_state, LUA_REGISTRYINDEX, functionRef);
    
    if(!error)
    {
        lua_pushlstring(lua_state, boost::asio::buffer_cast<const char *>(*buf->data().begin()), readlen);
        buf->consume(readlen);
        lua_pushnil(lua_state);
    }
    else
    {
        lua_pushnil(lua_state);
        lua_pushstring(lua_state, error.message().c_str());
    }
    
    if(lua_pcall(lua_state, 2, 0, 0) != 0)
        report_script_error(lua_tostring(lua_state, -1));
}

int socket_async_read_until(lua_State * L)
{
    buffered_tcp_socket * socket = reinterpret_cast<buffered_tcp_socket *>(lua_aux_checkobject(L, 1, BASIC_TCP_SOCKET_MT));

    const char * delim = luaL_checkstring(L, 2);
    
    luaL_checktype(L, 3, LUA_TFUNCTION);
    lua_pushvalue(L, 3);
    int functionRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    socket->async_read_until(delim, boost::bind(async_read_until_handler, functionRef, &socket->read_buffer(), _1, _2));
    
    return 0;
}

void async_read_handler(int functionRef, streambuf * socketbuf, lnetlib_buffer * rbuf, int bufferRef, int reqreadsize, const boost::system::error_code error, const size_t readsize)
{
    lua_rawgeti(lua_state, LUA_REGISTRYINDEX, functionRef);
    luaL_unref(lua_state, LUA_REGISTRYINDEX, functionRef);
    
    int nargs = 0;
    
    if(error)
    {
        lua_pushstring(lua_state, error.message().c_str());
        nargs = 1;
    }
    else
    {
        memcpy(rbuf->produced, buffer_cast<const void *>(socketbuf->data()), reqreadsize);
        socketbuf->consume(reqreadsize);
        rbuf->produced += reqreadsize;
    }
    
    if(lua_pcall(lua_state, nargs, 0, 0) != 0)
        report_script_error(lua_tostring(lua_state, -1));
    
    luaL_unref(lua_state, LUA_REGISTRYINDEX, bufferRef);
}

int socket_async_read(lua_State * L)
{
    buffered_tcp_socket * socket = reinterpret_cast<buffered_tcp_socket *>(lua_aux_checkobject(L, 1, BASIC_TCP_SOCKET_MT));
    lnetlib_buffer * rbuf = reinterpret_cast<lnetlib_buffer *>(luaL_checkudata(L, 2, BUFFER_MT));
    
    // Create reference to buffer to keep it alive
    lua_pushvalue(L, 2);
    int bufferRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    size_t reqreadsize = rbuf->get_write_left();
    
    int function_arg_index = 3;
    
    if(lua_type(L, 3) == LUA_TNUMBER)
    {
        reqreadsize = lua_tointeger(L, 3);
        if(reqreadsize > rbuf->get_write_left()) luaL_argerror(L, 3, "would cause overflow");
        function_arg_index = 4;
    }
    
    luaL_checktype(L, function_arg_index, LUA_TFUNCTION);
    lua_pushvalue(L, function_arg_index);
    int functionRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    socket->async_read(reqreadsize, boost::bind(async_read_handler, functionRef, &socket->read_buffer(), rbuf, bufferRef, reqreadsize, _1, _2));
    
    return 0;
}

void async_send_buffer_handler(int functionRef, lnetlib_buffer * buf, int bufferRef, const boost::system::error_code error, const size_t written)
{
    lua_rawgeti(lua_state, LUA_REGISTRYINDEX, functionRef);
    luaL_unref(lua_state, LUA_REGISTRYINDEX, functionRef);
    
    int nargs = 0;
    
    if(error)
    {
        lua_pushstring(lua_state, error.message().c_str());
        nargs = 1;
    }
    else buf->consumed += written;
    
    if(lua_pcall(lua_state, nargs, 0, 0) != 0)
        report_script_error(lua_tostring(lua_state, -1));
    
    luaL_unref(lua_state, LUA_REGISTRYINDEX, bufferRef);
}

int socket_async_send_buffer(lua_State * L)
{
    buffered_tcp_socket * socket = reinterpret_cast<buffered_tcp_socket *>(lua_aux_checkobject(L, 1, BASIC_TCP_SOCKET_MT));
    lnetlib_buffer * buf = reinterpret_cast<lnetlib_buffer *>(luaL_checkudata(L, 2, BUFFER_MT));
    
    lua_pushvalue(L, 2);
    int bufferRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    size_t writesize = buf->get_read_left();
    
    int function_arg_index = 3;
    
    if(lua_type(L, 3) == LUA_TNUMBER)
    {
        writesize = lua_tointeger(L, 3);
        function_arg_index = 4;
    }
    
    luaL_checktype(L, function_arg_index, LUA_TFUNCTION);
    lua_pushvalue(L, function_arg_index);
    int functionRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    socket->async_send(buffer(buf->consumed, writesize), boost::bind(async_send_buffer_handler, functionRef, buf, bufferRef, _1, _2));
    
    return 0;
}

int return_endpoint(lua_State * L, const boost::asio::ip::tcp::endpoint & endpoint, const boost::system::error_code error)
{
    if(error)
    {
        lua_pushnil(L);
        lua_pushstring(L, error.message().c_str());
        return 2;
    }
    
    push_endpoint(L, endpoint);
    
    return 1;
}

int socket_local_endpoint(lua_State * L)
{
    buffered_tcp_socket * socket = reinterpret_cast<buffered_tcp_socket *>(lua_aux_checkobject(L, 1, BASIC_TCP_SOCKET_MT));
    
    boost::system::error_code error;
    boost::asio::ip::tcp::endpoint endpoint = socket->local_endpoint(error);
    
    return return_endpoint(L, endpoint, error);
}

int socket_remote_endpoint(lua_State * L)
{
    buffered_tcp_socket * socket = reinterpret_cast<buffered_tcp_socket *>(lua_aux_checkobject(L, 1, BASIC_TCP_SOCKET_MT));
    
    boost::system::error_code error;
    boost::asio::ip::tcp::endpoint endpoint = socket->remote_endpoint(error);
    
    return return_endpoint(L, endpoint, error);
}

int socket_cancel(lua_State * L)
{
    reinterpret_cast<buffered_tcp_socket *>(lua_aux_checkobject(L, 1, BASIC_TCP_SOCKET_MT))->cancel();
    return 0;
}

int socket_shutdown_send(lua_State * L)
{
    reinterpret_cast<buffered_tcp_socket *>(lua_aux_checkobject(L, 1, BASIC_TCP_SOCKET_MT))->shutdown(ip::tcp::socket::shutdown_send);
    return 0;
}

int socket_shutdown_receive(lua_State * L)
{
    reinterpret_cast<buffered_tcp_socket *>(lua_aux_checkobject(L, 1, BASIC_TCP_SOCKET_MT))->shutdown(ip::tcp::socket::shutdown_receive);
    return 0;
}

int socket_shutdown(lua_State * L)
{
    reinterpret_cast<buffered_tcp_socket *>(lua_aux_checkobject(L, 1, BASIC_TCP_SOCKET_MT))->shutdown(ip::tcp::socket::shutdown_both);
    return 0;
}

int socket_set_option(lua_State * L)
{
    buffered_tcp_socket * socket = reinterpret_cast<buffered_tcp_socket *>(lua_aux_checkobject(L, 1, BASIC_TCP_SOCKET_MT));
    const char * optname = luaL_checkstring(L, 2);
    
    if(!strcmp(optname, "keep_alive"))
    {
        boost::asio::socket_base::keep_alive option(luaL_checkint(L, 3));
        socket->set_option(option);
    }
    else if(!strcmp(optname, "linger"))
    {
        boost::asio::socket_base::linger option(luaL_checkint(L, 3), luaL_checkint(L, 4));
        socket->set_option(option);
    }
    
    return 0;
}

int socket_get_option(lua_State * L)
{
    buffered_tcp_socket * socket = reinterpret_cast<buffered_tcp_socket *>(lua_aux_checkobject(L, 1, BASIC_TCP_SOCKET_MT));
    const char * optname = luaL_checkstring(L, 2);
    
    if(!strcmp(optname, "keep_alive"))
    {
        boost::asio::socket_base::keep_alive option;
        socket->get_option(option);
        lua_pushboolean(L, option.value());
        return 1;
    }
    else if(!strcmp(optname, "linger"))
    {
        boost::asio::socket_base::linger option;
        socket->get_option(option);
        
        lua_pushboolean(L, option.enabled());
        lua_pushinteger(L, option.timeout());
        return 2;
    }
    
    return 0;
}

void create_basic_socket_metatable(lua_State * L)
{
    luaL_newmetatable(L, BASIC_TCP_SOCKET_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    
    static luaL_Reg funcs[] = {
        {"__gc", socket_gc},
        {"close", socket_close},
        {"async_send", socket_async_send},
        {"async_read_until", socket_async_read_until},
        {"async_read", socket_async_read},
        {"local_endpoint", socket_local_endpoint},
        {"remote_endpoint", socket_remote_endpoint},
        {"cancel", socket_cancel},
        {"shutdown_send", socket_shutdown_send},
        {"shutdown_receive", socket_shutdown_receive},
        {"shutdown", socket_shutdown},
        {"set_option", socket_set_option},
        {"get_option", socket_get_option},
        {NULL, NULL}
    };
    
    luaL_register(L, NULL, funcs);
}

int udp_socket_gc(lua_State * L)
{
    reinterpret_cast<ip::udp::socket *>(lua_aux_checkobject(L, 1, UDP_SOCKET_MT))->~basic_datagram_socket<ip::udp>();
    return 0;
}

int udp_socket_close(lua_State * L)
{
    reinterpret_cast<ip::udp::socket *>(lua_aux_checkobject(L, 1, UDP_SOCKET_MT))->close();
    return 0;
}

int udp_socket_cancel(lua_State * L)
{
    reinterpret_cast<ip::udp::socket *>(lua_aux_checkobject(L, 1, UDP_SOCKET_MT))->cancel();
    return 0;
}

int udp_socket_bind(lua_State * L)
{
    ip::udp::socket * socket = reinterpret_cast<ip::udp::socket *>(lua_aux_checkobject(L, 1, UDP_SOCKET_MT));
    
    const char * ip = luaL_checkstring(L, 2);
    int port = luaL_checkint(L, 3);
    
    ip::udp::endpoint server_ep(ip::address_v4::from_string(ip), port);
    
    socket->open(server_ep.protocol());
    
    boost::system::error_code error;
    socket->bind(server_ep, error);
    
    if(error)
    {
        lua_pushstring(L, error.message().c_str());
        return 1;
    }
    else return 0;
}

void async_read_from_handler(int functionRef, lnetlib_buffer * rbuf, int bufferRef, ip::udp::endpoint * endpoint, const boost::system::error_code error, size_t readsize)
{
    lua_rawgeti(lua_state, LUA_REGISTRYINDEX, functionRef);
    luaL_unref(lua_state, LUA_REGISTRYINDEX, functionRef);
    
    int nargs = 1;
    
    if(error)
    {
        lua_pushnil(lua_state);
        lua_pushstring(lua_state, error.message().c_str());
        nargs = 2;
    }
    else
    {
        push_endpoint(lua_state, *endpoint);
        delete endpoint;
        
        rbuf->produced += readsize;
    }
    
    if(lua_pcall(lua_state, nargs, 0, 0) != 0)
        report_script_error(lua_tostring(lua_state, -1));
    
    luaL_unref(lua_state, LUA_REGISTRYINDEX, bufferRef);
}

int async_read_from(lua_State * L)
{
    ip::udp::socket * socket = reinterpret_cast<ip::udp::socket *>(lua_aux_checkobject(L, 1, UDP_SOCKET_MT));
    lnetlib_buffer * rbuf = reinterpret_cast<lnetlib_buffer *>(luaL_checkudata(L, 2, BUFFER_MT));
    
    lua_pushvalue(L, 2);
    int bufferRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    size_t reqreadsize = rbuf->get_write_left();
    
    int function_arg_index = 3;
    
    if(lua_type(L, 3) == LUA_TNUMBER)
    {
        reqreadsize = lua_tointeger(L, 3);
        if(reqreadsize > rbuf->get_write_left()) luaL_argerror(L, 3, "would cause overflow");
        function_arg_index = 4;
    }
    
    luaL_checktype(L, function_arg_index, LUA_TFUNCTION);
    lua_pushvalue(L, function_arg_index);
    int functionRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    ip::udp::endpoint * endpoint = new ip::udp::endpoint;
    
    socket->async_receive_from(buffer(rbuf->start, reqreadsize), *endpoint, boost::bind(async_read_from_handler, functionRef, rbuf, bufferRef, endpoint, _1, _2));
    
    return 0;
}

int async_send_to(lua_State * L)
{
    ip::udp::socket * socket = reinterpret_cast<ip::udp::socket *>(lua_aux_checkobject(L, 1, UDP_SOCKET_MT));
    lnetlib_buffer * buf = reinterpret_cast<lnetlib_buffer *>(luaL_checkudata(L, 2, BUFFER_MT));
    
    lua_pushvalue(L, 2);
    int bufferRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    size_t writesize = buf->get_read_left();
    
    int endpoint_arg_index = 3;
    int function_arg_index = 4;
    
    if(lua_type(L, 3) == LUA_TNUMBER)
    {
        writesize = lua_tointeger(L, 3);
        endpoint_arg_index = 4;
        function_arg_index = 5;
    }

    luaL_checktype(L, endpoint_arg_index, LUA_TTABLE);
    
    lua_getfield(L, endpoint_arg_index, "ip");
    if(lua_type(L, -1) != LUA_TSTRING) return luaL_argerror(L, endpoint_arg_index, "invalid ip for endpoint argument");
    const char * ip = lua_tostring(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, endpoint_arg_index, "port");
    if(!lua_isnumber(L, -1)) return luaL_argerror(L, endpoint_arg_index, "invalid port for endpoint argument");
    int port = lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    ip::udp::endpoint endpoint;
    
    try
    {
        endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(ip), port);
    }
    catch(const boost::system::system_error & error)
    {
        return luaL_argerror(L, endpoint_arg_index, error.code().message().c_str());
    }
    
    luaL_checktype(L, function_arg_index, LUA_TFUNCTION);
    lua_pushvalue(L, function_arg_index);
    int functionRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    socket->async_send_to(buffer(buf->consumed, writesize), endpoint, boost::bind(async_send_buffer_handler, functionRef, buf, bufferRef, _1, _2));
    
    return 0;
}

int udp_set_option(lua_State * L)
{
    ip::udp::socket * socket = reinterpret_cast<ip::udp::socket *>(luaL_checkudata(L, 1, UDP_SOCKET_MT));
    const char * optname = luaL_checkstring(L, 2);
    
    if(!strcmp(optname, "linger"))
    {
        boost::asio::socket_base::linger option(luaL_checkint(L, 3), luaL_checkint(L, 4));
        socket->set_option(option);
    }
    else if(!strcmp(optname, "broadcast"))
    {
        boost::asio::socket_base::broadcast option(luaL_checkint(L, 3));
        socket->set_option(option);
    }
    
    return 0;
}

int udp_get_option(lua_State * L)
{
    ip::udp::socket * socket = reinterpret_cast<ip::udp::socket *>(luaL_checkudata(L, 1, UDP_SOCKET_MT));
    const char * optname = luaL_checkstring(L, 2);
    
    if(!strcmp(optname, "linger"))
    {
        boost::asio::socket_base::enable_connection_aborted option;
        socket->get_option(option);
        lua_pushboolean(L, option.value());
        return 1;
    }
    else if(!strcmp(optname, "broadcast"))
    {
        boost::asio::socket_base::broadcast option;
        socket->get_option(option);
        lua_pushboolean(L, option.value());
        return 1;
    }
    
    return 0;
}

void create_udp_socket_metatable(lua_State * L)
{
    luaL_newmetatable(L, UDP_SOCKET_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    
    static luaL_Reg funcs[] = {
        {"__gc", udp_socket_gc},
        {"close", udp_socket_close},
        {"cancel", udp_socket_cancel},
        {"bind", udp_socket_bind},
        {"async_read_from", async_read_from},
        {"async_send_to", async_send_to},
        {"set_option", udp_set_option},
        {"get_option", udp_get_option},
        {NULL, NULL}
    };
    
    luaL_register(L, NULL, funcs);
}

int create_tcp_client(lua_State * L)
{
    buffered_tcp_socket * socket = new (lua_newuserdata(L, sizeof(buffered_tcp_socket))) buffered_tcp_socket(*main_io);
    
    // Setup metatable with __gc function set before bind() so the user-data object will be destroyed if the bind op fails.
    luaL_getmetatable(L, TCP_CLIENT_SOCKET_MT);
    lua_setmetatable(L, -2);
    
    socket->open(ip::tcp::v4());
    
    // Return user-data object
    return 1;
}

int create_udp_socket(lua_State * L)
{
    new (lua_newuserdata(L, sizeof(ip::udp::socket))) ip::udp::socket(*main_io);
    
    luaL_getmetatable(L, UDP_SOCKET_MT);
    lua_setmetatable(L, -2);
    
    return 1;
}

void async_connect_handler(int functionRef, const boost::system::error_code error)
{
    lua_rawgeti(lua_state, LUA_REGISTRYINDEX, functionRef);
    luaL_unref(lua_state, LUA_REGISTRYINDEX, functionRef);
    
    int nargs = 0;
    
    if(error)
    {
        lua_pushstring(lua_state, error.message().c_str());
        nargs = 1;
    }
    
    if(lua_pcall(lua_state, nargs, 0, 0) != 0)
        report_script_error(lua_tostring(lua_state, -1));
}

int client_async_connect(lua_State * L)
{
    buffered_tcp_socket * socket = reinterpret_cast<buffered_tcp_socket *>(lua_aux_checkobject(L, 1, TCP_CLIENT_SOCKET_MT));
    
    const char * ip = luaL_checkstring(L, 2);
    int port = luaL_checkinteger(L, 3);
    
    boost::asio::ip::tcp::endpoint endpoint;
    
    try
    {
        endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip), port);
    }
    catch(const boost::system::system_error & error)
    {
        return luaL_argerror(L, 2, error.code().message().c_str());
    }
    
    luaL_checktype(L, 4, LUA_TFUNCTION);
    
    lua_pushvalue(L, 4);
    int functionRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    if(!socket->is_open()) socket->open(ip::tcp::v4());
    
    socket->async_connect(endpoint, boost::bind(async_connect_handler, functionRef, _1));
    
    return 0;
}

void create_client_socket_metatable(lua_State * L)
{
    luaL_newmetatable(L, TCP_CLIENT_SOCKET_MT);
    
    luaL_getmetatable(L, BASIC_TCP_SOCKET_MT);
    lua_setmetatable(L, -2);
    
    static luaL_Reg funcs[] = {
        {"__gc", socket_gc},
        {"async_connect", client_async_connect},
        {NULL, NULL}
    };
    luaL_register(L, NULL, funcs);
    
    lua_setfield(L, -1, "__index");
}

int buffer_to_string(lua_State * L)
{
    lnetlib_buffer * buf = reinterpret_cast<lnetlib_buffer *>(luaL_checkudata(L, 1, BUFFER_MT));
    unsigned int len = buf->produced - buf->consumed;
    lua_pushlstring(L, buf->consumed, len);
    buf->consumed += len;
    return 1;
}

int buffer_size(lua_State * L)
{
    lnetlib_buffer * buf = reinterpret_cast<lnetlib_buffer *>(luaL_checkudata(L, 1, BUFFER_MT));
    lua_pushinteger(L, buf->get_size());
    return 1;
}

int buffer_read_left(lua_State * L)
{
    lnetlib_buffer * buf = reinterpret_cast<lnetlib_buffer *>(luaL_checkudata(L, 1, BUFFER_MT));
    lua_pushinteger(L, buf->get_read_left());
    return 1;
}

int buffer_write_left(lua_State * L)
{
    lnetlib_buffer * buf = reinterpret_cast<lnetlib_buffer *>(luaL_checkudata(L, 1, BUFFER_MT));
    lua_pushinteger(L, buf->get_write_left());
    return 1;
}

int buffer_read_uint8(lua_State * L)
{
    lnetlib_buffer * buf = reinterpret_cast<lnetlib_buffer *>(luaL_checkudata(L, 1, BUFFER_MT));
    
    if(buf->consumed == buf->produced) return 0;
    
    unsigned char value = *buf->consumed;
    buf->consumed += 1;
    
    lua_pushinteger(L, value);
    
    return 1;
}

int buffer_read_uint16(lua_State * L)
{
    lnetlib_buffer * buf = reinterpret_cast<lnetlib_buffer *>(luaL_checkudata(L, 1, BUFFER_MT));
    
    if(buf->consumed + 2 > buf->produced) return 0;
    
    char * data = buf->consumed;
    unsigned char hi = *data;
    unsigned char lo = *(data + 1);
    unsigned short value = (hi << 8) + lo;
    buf->consumed += 2;
    
    lua_pushinteger(L, value);
    
    return 1;
}

int buffer_read_uint32(lua_State * L)
{
    lnetlib_buffer * buf = reinterpret_cast<lnetlib_buffer *>(luaL_checkudata(L, 1, BUFFER_MT));
    
    if(buf->consumed + 4 > buf->produced) return 0;
    
    char * data = buf->consumed;
    unsigned char b1 = *data;
    unsigned char b2 = *(data + 1);
    unsigned char b3 = *(data + 2);
    unsigned char b4 = *(data + 3);
    unsigned long value = (b1 << 24)  + (b2 << 16) + (b3 << 8) + b4;
    buf->consumed += 4;
    
    lua_pushinteger(L, value);
    
    return 1;
}

int buffer_read_string(lua_State * L)
{
    lnetlib_buffer * buf = reinterpret_cast<lnetlib_buffer *>(luaL_checkudata(L, 1, BUFFER_MT));
    int len = luaL_checkint(L, 2);
    if(buf->consumed + len > buf->produced) return 0;
    lua_pushlstring(L, buf->consumed, len);
    buf->consumed += len;
    return 1;
}

int buffer_reset(lua_State * L)
{
    lnetlib_buffer * buf = reinterpret_cast<lnetlib_buffer *>(luaL_checkudata(L, 1, BUFFER_MT));
    buf->consumed = buf->start;
    buf->produced = buf->start;
    return 0;
}

int buffer_write_uint8(lua_State * L)
{
    lnetlib_buffer * buf = reinterpret_cast<lnetlib_buffer *>(luaL_checkudata(L, 1, BUFFER_MT));
    if(buf->produced == buf->end) return 0;    
    int value = luaL_checkint(L, 2);
    *buf->produced = value;
    buf->produced++;
    lua_pushboolean(L, 1);
    return 1;
}

int buffer_write_uint16(lua_State * L)
{
    lnetlib_buffer * buf = reinterpret_cast<lnetlib_buffer *>(luaL_checkudata(L, 1, BUFFER_MT));
    if(buf->produced + 2 > buf->end) return 0;
    unsigned short value = luaL_checkint(L, 2);
    *buf->produced = value >> 8;
    *(buf->produced + 1) = value & 0xFF;
    buf->produced += 2;
    lua_pushboolean(L, 1);
    return 1;
}

int buffer_write_uint32(lua_State * L)
{
    lnetlib_buffer * buf = reinterpret_cast<lnetlib_buffer *>(luaL_checkudata(L, 1, BUFFER_MT));
    if(buf->produced + 4 > buf->end) return 0;    
    int value = luaL_checkint(L, 2);
    *buf->produced = value >> 24;
    *(buf->produced + 1) = (value >> 16) & 0xFF;
    *(buf->produced + 2) = (value >> 8) & 0xFF;
    *(buf->produced + 3) = value & 0xFF; 
    buf->produced++;
    lua_pushboolean(L, 1);
    return 1;
}

int buffer_write_string(lua_State * L)
{
    lnetlib_buffer * buf = reinterpret_cast<lnetlib_buffer *>(luaL_checkudata(L, 1, BUFFER_MT));
    size_t len;
    const char * str = luaL_checklstring(L, 2, &len);
    if(buf->produced + len > buf->end) return 0;
    strncpy(buf->produced, str, len);
    buf->produced += len;
    lua_pushboolean(L, 1);
    return 1;
}

int create_buffer(lua_State * L)
{
    int size = luaL_checkint(L, 1);
    if(size <= 0) return luaL_argerror(L, 1, "invalid size");
    
    void * bufalloc = lua_newuserdata(L, size + sizeof(lnetlib_buffer));
    luaL_getmetatable(L, BUFFER_MT);
    lua_setmetatable(L, -2);
    
    lnetlib_buffer * rbuf = reinterpret_cast<lnetlib_buffer *>(bufalloc);
    rbuf->start = reinterpret_cast<char *>(rbuf) + sizeof(lnetlib_buffer);
    rbuf->end = rbuf->start + size;
    rbuf->produced = rbuf->start;
    rbuf->consumed = rbuf->start;
    
    //assert(reinterpret_cast<unsigned int>(rbuf) % sizeof(unsigned int) == 0);//check for correct memory alignment
    
    return 1;
}

void create_buffer_metatable(lua_State * L)
{
    luaL_newmetatable(L, BUFFER_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    
    static luaL_Reg funcs[] = {
        {"reset", buffer_reset},
        {"to_string", buffer_to_string},
        {"size", buffer_size},
        {"read_left", buffer_read_left},
        {"read_uint8", buffer_read_uint8},
        {"read_uint16", buffer_read_uint16},
        {"read_uint32", buffer_read_uint32},
        {"read_string", buffer_read_string},
        {"write_left", buffer_write_left},
        {"write_uint8", buffer_write_uint8},
        {"write_uint16", buffer_write_uint16},
        {"write_uint32", buffer_write_uint32},
        {"write_string", buffer_write_string},
        {NULL, NULL}
    };
    
    luaL_register(L, NULL, funcs);
}

int local_acceptor_gc(lua_State * L)
{
    local::stream_protocol::acceptor * acceptor = reinterpret_cast<local::stream_protocol::acceptor *>(luaL_checkudata(L, 1, LOCAL_ACCEPTOR_MT));
    unlink(acceptor->local_endpoint().path().c_str());
    acceptor->~basic_socket_acceptor<local::stream_protocol>();
    return 0;
}

int local_acceptor_close(lua_State * L)
{
    local::stream_protocol::acceptor * acceptor = reinterpret_cast<local::stream_protocol::acceptor *>(luaL_checkudata(L, 1, LOCAL_ACCEPTOR_MT));
    acceptor->close();
    return 0;
}

int local_acceptor_listen(lua_State * L)
{
    local::stream_protocol::acceptor * acceptor = reinterpret_cast<local::stream_protocol::acceptor *>(luaL_checkudata(L, 1, LOCAL_ACCEPTOR_MT));
    acceptor->listen();
    return 0;
}

int local_acceptor_async_accept(lua_State * L)
{
    luaL_checktype(L, 2, LUA_TFUNCTION);
    local::stream_protocol::acceptor * acceptor = reinterpret_cast<local::stream_protocol::acceptor *>(luaL_checkudata(L, 1, LOCAL_ACCEPTOR_MT));
    
    buffered_local_socket * socket = new (lua_newuserdata(L, sizeof(buffered_local_socket))) buffered_local_socket(*main_io);
    luaL_getmetatable(L, LOCAL_SOCKET_MT);
    lua_setmetatable(L, -2);
    int socketRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    lua_pushvalue(L, 2);
    acceptor->async_accept(*socket, boost::bind(async_accept_handler, luaL_ref(L, LUA_REGISTRYINDEX), socketRef, _1));
    
    return 0;
}

int create_local_acceptor(lua_State * L)
{
    const char * path = luaL_checkstring(L, 1);
    
    local::stream_protocol::acceptor * acceptor = new (lua_newuserdata(L, sizeof(local::stream_protocol::acceptor))) local::stream_protocol::acceptor(*main_io);
    
    // Setup metatable with __gc function set before bind() so the user-data object will be destroyed if the bind op fails.
    luaL_getmetatable(L, LOCAL_ACCEPTOR_MT);
    lua_setmetatable(L, -2);
    
    local::stream_protocol::endpoint endpoint(path);
    
    acceptor->open(endpoint.protocol());

    boost::system::error_code error;
    acceptor->bind(endpoint, error);
    if(error)
    {
        lua_pushnil(L);
        lua_pushstring(L, error.message().c_str());
        return 2;
    }
    
    // Return user-data object
    return 1;
}

void create_local_acceptor_metatable(lua_State * L)
{
    luaL_newmetatable(L, LOCAL_ACCEPTOR_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    
    static luaL_Reg funcs[] = {
        {"__gc", local_acceptor_gc},
        {"close", local_acceptor_close},
        {"listen", local_acceptor_listen},
        {"async_accept", local_acceptor_async_accept},
        {NULL, NULL}
    };
    
    luaL_register(L, NULL, funcs);
}

int local_socket_gc(lua_State * L)
{
    reinterpret_cast<buffered_local_socket *>(lua_aux_checkobject(L, 1, LOCAL_SOCKET_MT))->~buffered_local_socket();
    return 0;
}

int local_socket_close(lua_State * L)
{
    reinterpret_cast<buffered_local_socket *>(lua_aux_checkobject(L, 1, LOCAL_SOCKET_MT))->close();
    return 0;
}

int local_socket_cancel(lua_State * L)
{
    reinterpret_cast<buffered_local_socket *>(lua_aux_checkobject(L, 1, LOCAL_SOCKET_MT))->cancel();
    return 0;
}

int local_socket_shutdown_send(lua_State * L)
{
    reinterpret_cast<buffered_local_socket *>(lua_aux_checkobject(L, 1, LOCAL_SOCKET_MT))->shutdown(buffered_local_socket::shutdown_send);
    return 0;
}

int local_socket_shutdown_receive(lua_State * L)
{
    reinterpret_cast<buffered_local_socket *>(lua_aux_checkobject(L, 1, LOCAL_SOCKET_MT))->shutdown(buffered_local_socket::shutdown_receive);
    return 0;
}

int local_socket_shutdown(lua_State * L)
{
    reinterpret_cast<buffered_local_socket *>(lua_aux_checkobject(L, 1, LOCAL_SOCKET_MT))->shutdown(buffered_local_socket::shutdown_both);
    return 0;
}

int local_socket_async_send_buffer(lua_State * L)
{
    buffered_local_socket * socket = reinterpret_cast<buffered_local_socket *>(lua_aux_checkobject(L, 1, LOCAL_SOCKET_MT));
    lnetlib_buffer * buf = reinterpret_cast<lnetlib_buffer *>(luaL_checkudata(L, 2, BUFFER_MT));
    
    lua_pushvalue(L, 2);
    int bufferRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    size_t writesize = buf->get_read_left();
    
    int function_arg_index = 3;
    
    if(lua_type(L, 3) == LUA_TNUMBER)
    {
        writesize = lua_tointeger(L, 3);
        function_arg_index = 4;
    }
    
    luaL_checktype(L, function_arg_index, LUA_TFUNCTION);
    lua_pushvalue(L, function_arg_index);
    int functionRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    socket->async_send(buffer(buf->consumed, writesize), boost::bind(async_send_buffer_handler, functionRef, buf, bufferRef, _1, _2));
    
    return 0;
}

int local_socket_async_send(lua_State * L)
{
    buffered_local_socket * socket = reinterpret_cast<buffered_local_socket  *>(lua_aux_checkobject(L, 1, LOCAL_SOCKET_MT));
    
    if(lua_type(L, 2) != LUA_TSTRING) return local_socket_async_send_buffer(L);
    
    luaL_checktype(L, 3, LUA_TFUNCTION);
    
    size_t stringlen;
    const char * str = lua_tolstring(L, 2, &stringlen);
    lua_pushvalue(L, -1);
    int stringRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    lua_pushvalue(L, 3);
    socket->async_send(boost::asio::buffer(str, stringlen), boost::bind(async_send_handler, luaL_ref(L, LUA_REGISTRYINDEX), stringRef, _1, _2));
    
    return 0;
}

int local_socket_async_read_until(lua_State * L)
{
    buffered_local_socket * socket = reinterpret_cast<buffered_local_socket *>(lua_aux_checkobject(L, 1, LOCAL_SOCKET_MT));

    const char * delim = luaL_checkstring(L, 2);
    
    luaL_checktype(L, 3, LUA_TFUNCTION);
    lua_pushvalue(L, 3);
    int functionRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    boost::asio::streambuf * readbuf = new boost::asio::streambuf;
    
    socket->async_read_until(delim, boost::bind(async_read_until_handler, functionRef, readbuf, _1, _2));
    
    return 0;
}

int local_socket_async_read(lua_State * L)
{
    buffered_local_socket * socket = reinterpret_cast<buffered_local_socket *>(lua_aux_checkobject(L, 1, LOCAL_SOCKET_MT));
    lnetlib_buffer * rbuf = reinterpret_cast<lnetlib_buffer *>(luaL_checkudata(L, 2, BUFFER_MT));
    
    lua_pushvalue(L, 2);
    int bufferRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    size_t reqreadsize = rbuf->get_write_left();
    
    int function_arg_index = 3;
    
    if(lua_type(L, 3) == LUA_TNUMBER)
    {
        reqreadsize = lua_tointeger(L, 3);
        if(reqreadsize > rbuf->get_write_left()) luaL_argerror(L, 3, "would cause overflow");
        function_arg_index = 4;
    }
    
    luaL_checktype(L, function_arg_index, LUA_TFUNCTION);
    lua_pushvalue(L, function_arg_index);
    int functionRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    socket->async_read(reqreadsize, boost::bind(async_read_handler, functionRef, &socket->read_buffer(), rbuf, bufferRef, reqreadsize, _1, _2));
    
    return 0;
}

int local_socket_local_endpoint(lua_State * L)
{
    buffered_local_socket * socket = reinterpret_cast<buffered_local_socket *>(lua_aux_checkobject(L, 1, LOCAL_SOCKET_MT));
    boost::system::error_code error;
    local::stream_protocol::endpoint endpoint = socket->local_endpoint(error);
    if(error) return 0;
    lua_pushstring(L, endpoint.path().c_str());
    return 1;
}

int local_socket_remote_endpoint(lua_State * L)
{
    buffered_local_socket * socket = reinterpret_cast<buffered_local_socket *>(lua_aux_checkobject(L, 1, LOCAL_SOCKET_MT));
    boost::system::error_code error;
    local::stream_protocol::endpoint endpoint = socket->remote_endpoint(error);
    if(error) return 0;
    lua_pushstring(L, endpoint.path().c_str());
    return 1;
}

void create_local_socket_metatable(lua_State * L)
{
    luaL_newmetatable(L, LOCAL_SOCKET_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    
    static luaL_Reg funcs[] = {
        {"__gc", local_socket_gc},
        {"close", local_socket_close},
        {"async_send", local_socket_async_send},
        {"async_read_until", local_socket_async_read_until},
        {"async_read", local_socket_async_read},
        {"local_endpoint", local_socket_local_endpoint},
        {"remote_endpoint", local_socket_remote_endpoint},
        {"cancel", local_socket_cancel},
        {"shutdown_send", local_socket_shutdown_send},
        {"shutdown_receive", local_socket_shutdown_receive},
        {"shutdown", local_socket_shutdown},
        {NULL, NULL}
    };
    
    luaL_register(L, NULL, funcs);
}

int local_socket_client_async_connect(lua_State * L)
{
    buffered_local_socket * socket = reinterpret_cast<buffered_local_socket *>(lua_aux_checkobject(L, 1, LOCAL_SOCKET_CLIENT_MT));
    
    const char * path = luaL_checkstring(L, 2);

    local::stream_protocol::endpoint endpoint;
    
    try
    {
        endpoint = local::stream_protocol::endpoint(path);
    }
    catch(const boost::system::system_error & error)
    {
        return luaL_argerror(L, 2, error.code().message().c_str());
    }
    
    luaL_checktype(L, 3, LUA_TFUNCTION);
    
    lua_pushvalue(L, 3);
    int functionRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    //if(!socket->is_open()) socket->open(local::stream_protocol::endpoint::protocol());
    
    socket->async_connect(endpoint, boost::bind(async_connect_handler, functionRef, _1));
    
    return 0;
}

void create_local_socket_client_metatable(lua_State * L)
{
    luaL_newmetatable(L, LOCAL_SOCKET_CLIENT_MT);
    lua_pushvalue(L, -1);
    
    luaL_getmetatable(L, LOCAL_SOCKET_MT);
    lua_setmetatable(L, -2);
    
    static luaL_Reg funcs[] = {
        {"__gc", local_socket_gc},
        {"async_connect", local_socket_client_async_connect},
        {NULL, NULL}
    };
    
    luaL_register(L, NULL, funcs);
    
    lua_setfield(L, -1, "__index");
}

int create_local_client(lua_State * L)
{
    new (lua_newuserdata(L, sizeof(buffered_local_socket))) buffered_local_socket(*main_io);
    
    // Setup metatable with __gc function set before bind() so the user-data object will be destroyed if the bind op fails.
    luaL_getmetatable(L, LOCAL_SOCKET_CLIENT_MT);
    lua_setmetatable(L, -2);
    
    // Return user-data object
    return 1;
}


void register_lnetlib()
{
    lua_state = get_script_env().get_lua_state();
    main_io = &get_main_io_service();
    
    create_acceptor_metatable(lua_state);
    create_basic_socket_metatable(lua_state);
    create_client_socket_metatable(lua_state);
    create_buffer_metatable(lua_state);
    create_udp_socket_metatable(lua_state);
    create_local_acceptor_metatable(lua_state);
    create_local_socket_metatable(lua_state);
    create_local_socket_client_metatable(lua_state);
    
    static luaL_Reg net_funcs[] = {
        {"async_resolve", async_resolve},
        {"tcp_acceptor", create_tcp_acceptor},
        {"tcp_client", create_tcp_client},
        {"udp_socket", create_udp_socket},
        {"local_acceptor", create_local_acceptor},
        {"local_client", create_local_client},
        {"buffer", create_buffer},
        {NULL, NULL}
    };
    
    luaL_register(lua_state, "net", net_funcs);
}

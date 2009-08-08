/*   
 *   The Fungu Scripting Engine
 *   
 *   Copyright (c) 2008-2009 Graham Daws.
 *
 *   Distributed under a BSD style license (see accompanying file LICENSE.txt)
 */

namespace fungu{
namespace script{

any_variable::any_variable()
 :m_procedure(false)
{
    
}

env::object::object_type any_variable::get_object_type()const
{
    if(m_procedure) 
        return any_cast<env::object::shared_ptr>(m_any)->get_object_type();
    else return DATA_OBJECT;
}

void any_variable::assign(const any & value)
{
    m_any = value;
    m_procedure = value.get_type() == typeid(env::object::shared_ptr);
    if(!m_procedure && any_is_string(m_any))
        m_any = const_string(m_any.to_string().copy());
}

result_type any_variable::call(call_arguments & args,env::frame * frame)
{
    if(m_procedure)
        return any_cast<env::object::shared_ptr>(m_any)->call(args,frame);
    else
    {
        assign(args.safe_front());
        args.pop_front();
        return m_any;
    }
    
    return false;
}

#ifdef FUNGU_WITH_LUA
int any_variable::call(lua_State * L)
{
    if(m_procedure)
        return any_cast<env::object::shared_ptr>(m_any)->call(L);
    else return luaL_error(L, "not a function");
}
#endif

result_type any_variable::value()
{
    if(m_procedure)
        return any_cast<env::object::shared_ptr>(m_any)->value();
    return m_any;
}

env::object * any_variable::lookup_member(const_string id)
{
    if(m_procedure)
        return any_cast<shared_ptr>(m_any)->lookup_member(id);
    else return NULL; 
}

const any & any_variable::get_any()const
{
    return m_any;
}

} //namespace script
} //namespace fungu
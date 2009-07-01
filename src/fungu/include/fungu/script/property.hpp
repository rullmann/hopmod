/*   
 *   The Fungu Scripting Engine Library
 *   
 *   Copyright (c) 2008-2009 Graham Daws.
 *
 *   Distributed under a BSD style license (see accompanying file LICENSE.txt)
 */
#ifndef FUNGU_SCRIPT_PROPRETY_HPP
#define FUNGU_SCRIPT_PROPERTY_HPP

#include "env.hpp"

namespace fungu{
namespace script{

template<typename T>
class property:public env::object
{
public:
    template<typename GetterFunction, typename SetterFunction>
    property(GetterFunction getter, SetterFunction setter)
      :m_get(getter),
       m_set(setter)
    {
        
    }

    object_type get_object_type()const
    {
        return DATA_OBJECT;
    }
    
    void assign(const any & value)
    {
        m_set(lexical_cast<T>(value));
    }
    
    result_type call(call_arguments & args,env::frame *)
    {
        assign(args.safe_front());
        return any();
    }
    
    result_type value()
    {
        return m_get();
    }
    
    static T generic_getter(const T & value)
    {
        return value;
    }
    
    static void generic_setter(T & dst, const any & src)
    {
        dst = lexical_cast<T>(src);
    }
private:
    boost::function0<T> m_get;
    boost::function1<void,T> m_set;
};

} //namespace script
} //namespace fungu

#endif

#pragma once

template<class T>
struct del_fun_t 
{
   del_fun_t& operator()(T* p) { 
     delete p;
     return *this;
   }
};

template<class T>
del_fun_t<T> del_fun() 
{ 
   return del_fun_t<T>(); 
}

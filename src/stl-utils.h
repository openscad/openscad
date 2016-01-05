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

template<class T>
struct reset_fun_t
{
	reset_fun_t& operator()(T p) {
		p.reset();
		return *this;
	}
};

template<class T>
reset_fun_t<T> reset_fun()
{
	return reset_fun_t<T>();
}

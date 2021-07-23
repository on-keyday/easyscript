#include<utility>
namespace commonlib{
    template<class T>
    struct get_array_elm_impl{
        T t;
        using type=decltype(t[0]);
    };

    template<class T>
    using array_elm_t=typename get_array_elm_impl<T>::type;

    template<class T>
    using arrelm_remove_cvref_t=std::remove_cvref_t<array_elm_t<T>>;

    template<class T,class U,bool>
    struct larger_type_impl;

    template<class T,class U>
    struct larger_type_impl<T,U,true>{
        using type=T;
    };

    template<class T,class U>
    struct larger_type_impl<T,U,false>{
        using type=U;
    };

    template<class T,class U>
    using larger_type=typename larger_type_impl<T,U,(sizeof(T)>sizeof(U))>::type;
}

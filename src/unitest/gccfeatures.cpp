// gccfeatures.cpp --- 
// 
// Author: liuguangzhao
// Copyright (C) 2007-2010 liuguangzhao@users.sf.net
// URL: 
// Created: 2009-10-18 09:07:27 +0800
// Version: $Id$
// 

#include <stdio.h>


#include "gccfeatures.h"

void gcc_feature_statements_and_declarations_in_expressions()
{
    int ia;
    ia = ({
        int y = 5;
        int z;
        if (y > 0) z = y;
        else z= -y;
        z;
    });
    printf("a=%d\n", ia);

    // for classes
    // 还可以在一个函数内定义一个类。
    class A {
    public:
        void  foo() {
            printf("it's A.foo()\n");
        }
    };
    
    A ca;
    ({ca;}).foo();

    int la,lb;
#define maxint_sd_express(a,b)   ({        \
            __typeof__(a) __return_value;  \
            if (a > b) __return_value = a; \
            else __return_value = b;       \
            __return_value;                \
        })

    la = 6, lb = 3;
    int maxv = maxint_sd_express(la, lb);
    printf("maxint of %d,%d is %d=?%d\n", la, lb, maxv, maxint_sd_express(lb, la));
    
    // 这代码就够复杂了，如果不知道这个扩展，看着也头晕。

    // c++0x test
#if ((__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)) && __GXX_EXPERIMENTAL_CXX0X__)
#else
#endif
}


void gcc_feature___atomic_rather_than___sync()
{

    //   __atomic();

}

namespace gcc_ext_friend {
    template<class W>
    class TQ {
        static const int I = 2;
    public:
        friend W;
    };

    struct TB {
        int ar[TQ<TB>::I];
    };

    void gcc_ext_friend()
    {

    }
};

namespace gcc_ext_delegate_construct {
    struct A {
        A(int);
        A() : A(30) {}
    };
};

#include <atomic>
#include <iostream>
namespace gcc_ext_atomic_class {
    struct A {
        int a;
        // std::string astr;
        FILE *fp;
    };
    class POD {
        int a;
        int b;
        // std::string str; // compile error
        char *rstr;
        char fstr[100];
        A a2;
    };
    std::atomic<POD> my_atomic_POD;
};

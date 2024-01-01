/**
 * @file   demangle.cpp
 * @date   Mon Dec 12 05:40:40 2022
 * 
 * @brief  test bugs:
    @li capture does not demangle .info result
    @li duplicate function records in result
 */

#include<iostream>

class Animal
{
public:
    Animal()
    {   
        std::cout << "Animal" <<std::endl;
    } 
    virtual ~Animal()
    {   
      std::cout << "~Animal" <<std::endl;
    }   
};

class Cat : public Animal
{
public:
    Cat()
    {   
        std::cout << "Cat" <<std::endl;
    }   
    ~Cat()
    {   
        std::cout << "~Cat" <<std::endl;
    }   
};

int main()
{
    Animal* animal = new Cat;
    delete animal;
    Cat* cat = new Cat;
    delete cat;
    Cat cat2;
}

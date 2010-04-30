#include "Interval.hh"
#include <iostream>

std::ostream &operator <<(std::ostream &out, const Interval& t) 
{
    return (out << (double)t);
}

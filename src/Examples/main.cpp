
#include "xcore_examples.h"

#include <cstdlib>
#include <new>

//------------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------------
int main(void)
{
    xcore::Init("TestApp");
    xcore::examples::Test();

    return 0;
}
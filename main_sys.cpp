
#include <ta_libc.h>

#include <string>
#include <iostream>
#include <assert.h>

#include "src/defs.h"


int main(int argc, char* argv[])
{
    TA_RetCode retCode;
    int outBegIdx = 0;
    int outNBElement = 0;

    retCode = TA_Initialize();
    if (retCode != TA_SUCCESS)
    {
        std::cout << "TA-Lib initialize error: " << retCode << std::endl;
        return retCode;
    }
    std::cout << "TA-Lib initialized.\n";


    assert(INTERVAL_MAP.size() == NUM_INTERVALS);



    retCode = TA_Shutdown();
    if (retCode != TA_SUCCESS)
    {
        std::cout << "TA-Lib shutdown error: " << retCode << std::endl;
        return retCode;
    }
    return 0;
}

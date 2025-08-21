#ifndef JK_BMS_H
#define JK_BMS_H

#include "models/constants.h"
#include "models/battery_info.h"
#include "models/settings_info.h"
#include "models/cell_info.h"

class JKBMS
{
public:
    JKBMS();
    ~JKBMS();

    void begin();
    void update();

private:
    char bluetoothBuffer[2048];
};


#endif // JK_BMS_H
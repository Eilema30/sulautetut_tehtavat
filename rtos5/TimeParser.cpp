#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "TimeParser.h"

// time format: HHMMSS (6 characters)
int time_parse(char *time) 
{
	// Check that string is not NULL
	if (time == NULL) {
        return TIME_ARRAY_ERROR;
    }

    // Check that string is 6 characters long
    if (strlen(time) != 6) {
        return TIME_LEN_ERROR;
    }

    // Check that all characters are numbers
    for (int i = 0; i < 6; i++) {
        if (!isdigit((unsigned char)time[i])) {
            return TIME_VALUE_ERROR;
        }
    }

    // Parse values from time string
    int hours = (time[0] - '0') * 10 + (time[1] - '0');
    int minutes = (time[2] - '0') * 10 + (time[3] - '0');
    int seconds = (time[4] - '0') * 10 + (time[5] - '0');

    // Check boundary values
    if (hours < 0 || hours > 23) {
        return TIME_VALUE_ERROR;
    }

    if (minutes < 0 || minutes > 59) {
        return TIME_VALUE_ERROR;
    }

    if (seconds < 0 || seconds > 59) {
        return TIME_VALUE_ERROR;
    }

    // 0 seconds = timer useless
    /*if (hours == 0 && minutes == 0 && seconds == 0) {
        return TIME_VALUE_ERROR;
    }*/

    // Convert minutes and seconds to total seconds
    int total_seconds = minutes * 60 + seconds;

    return total_seconds;
}

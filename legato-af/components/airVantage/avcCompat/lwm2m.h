/**
 * @file lwm2m.h
 *
 * Interface for LWM2M handler sub-component
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#ifndef LEGATO_LWM2M_INCLUDE_GUARD
#define LEGATO_LWM2M_INCLUDE_GUARD

#include "legato.h"

//--------------------------------------------------------------------------------------------------
// Definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Interface functions
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Init this sub-component
 */
//--------------------------------------------------------------------------------------------------
le_result_t lwm2m_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Checks if read operation notifications is received
 */
//--------------------------------------------------------------------------------------------------
bool lwm2m_IsReadEventReceived
(
    void
);

#endif // LEGATO_LWM2M_INCLUDE_GUARD
#include "le_fwupdate_interface.h"
#include "le_flash_interface.h"
#include "le_dualsys_interface.h"
#include "le_cfg_interface.h"

#undef LE_KILL_CLIENT
#define LE_KILL_CLIENT LE_WARN

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_flash_GetClientSessionRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_flash_GetServiceRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Sets the handler callback function to be called when the session is closed from the other
 * end.  A local termination of the session will not trigger this callback.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t le_msg_AddServiceCloseHandler
(
    le_msg_ServiceRef_t             serviceRef, ///< [in] Reference to the service.
    le_msg_SessionEventHandler_t    handlerFunc,///< [in] Handler function.
    void*                           contextPtr  ///< [in] Opaque pointer value to pass to handler.
);

//--------------------------------------------------------------------------------------------------
/**
 * Wakeup source reference
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_pm_WakeupSourceRef_t);

//--------------------------------------------------------------------------------------------------
/**
 * Try to connect to PowerManager service
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pm_TryConnectService
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Disconnect from PowerManager service
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pm_DisconnectService
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Create a new wakeup source
 */
//--------------------------------------------------------------------------------------------------
le_pm_WakeupSourceRef_t le_pm_NewWakeupSource
(
    uint32_t opts,      ///< [in] Wakeup source options
    const char *tag     ///< [in] Context-specific wakeup source tag
);

//--------------------------------------------------------------------------------------------------
/**
 * Acquire a wakeup source
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pm_StayAwake
(
    le_pm_WakeupSourceRef_t w   ///< [in] Wakeup source reference
);

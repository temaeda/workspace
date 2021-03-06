//--------------------------------------------------------------------------------------------------
/**
 *  Data Channel Server's C code implementation of its southbound interfaces with the
 *  technology-specific handlers and APIs.  This implementation may get significantly revised
 *  or even eliminated when we make the southbound components plug-and-play.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include "legato.h"
#include "interfaces.h"
#include "le_print.h"
#include "pa_mdc.h"
#include "pa_dcs.h"
#include "dcs.h"
#include "dcsNet.h"
#include "dcsCellular.h"
#ifdef LE_CONFIG_ENABLE_WIFI
#include "dcsWifi.h"
#endif
#ifdef LE_CONFIG_ENABLE_ETHERNET
#include "dcsEthernet.h"
#endif


//--------------------------------------------------------------------------------------------------
/**
 * The following is the global data struct for running a channel list query & collecting its results
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dcs_ChannelInfo_t list[LE_DCS_CHANNEL_LIST_QUERY_MAX]; ///< list of channels collected
    uint16_t listSize;                                        ///< size of the list being filled
    uint8_t techPending[LE_DCS_TECH_MAX];                     ///< flags indicating each tech
                                                              ///< pending for query results or not
} DcsQueryChannel_t;

static DcsQueryChannel_t QueryChannel;


//--------------------------------------------------------------------------------------------------
/**
 * Create a technology specific channel db for the given channel according to its given
 * technology in its 1st argument
 *
 * @return
 *     - Return in the function's return value a void * pointer to this newly created db's object
 *       reference
 *     - Return NULL back if the creation has failed
 */
//--------------------------------------------------------------------------------------------------
void *dcsTech_CreateTechRef
(
    le_dcs_Technology_t tech,
    const char *channelName
)
{
    void *techRef = NULL;

    switch (tech)
    {
        case LE_DCS_TECH_CELLULAR:
            techRef = le_dcsCellular_CreateConnDb(channelName);
            break;
#ifdef LE_CONFIG_ENABLE_WIFI
        case LE_DCS_TECH_WIFI:
            techRef = le_dcsWifi_CreateConnDb(channelName);
            break;
#endif
#ifdef LE_CONFIG_ENABLE_ETHERNET
        case LE_DCS_TECH_ETHERNET:
            techRef = le_dcsEthernet_CreateConnDb(channelName);
            break;
#endif
        default:
            LE_ERROR("Unsupported technology %d", tech);
    }
    return techRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Release a technology specific channel db for the given channel according to its given
 * technology in its 1st argument
 */
//--------------------------------------------------------------------------------------------------
void dcsTech_ReleaseTechRef
(
    le_dcs_Technology_t tech,
    void *techRef
)
{
    switch (tech)
    {
        case LE_DCS_TECH_CELLULAR:
            le_dcsCellular_ReleaseConnDb(techRef);
            break;
#ifdef LE_CONFIG_ENABLE_WIFI
        case LE_DCS_TECH_WIFI:
            le_dcsWifi_ReleaseConnDb(techRef);
            break;
#endif
#ifdef LE_CONFIG_ENABLE_ETHERNET
        case LE_DCS_TECH_ETHERNET:
            le_dcsEthernet_ReleaseConnDb(techRef);
            break;
#endif
        default:
            LE_ERROR("Unsupported technology %d", tech);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function checks if DCS is still pending for any technology to return its list of available
 * channels. If the input argument is a given technology type, this function just checks this one
 * only. If it is LE_DCS_TECH_MAX, it checks all technology types.
 *
 * @return
 *     - True if DCS is pending; otherwise, false
 */
//--------------------------------------------------------------------------------------------------
bool dcsTech_ChannelQueryIsPending
(
    le_dcs_Technology_t tech
)
{
    uint16_t i;

    if (tech < LE_DCS_TECH_MAX)
    {
        return QueryChannel.techPending[tech];
    }

    for (i=1; i<LE_DCS_TECH_MAX; i++)
    {
        if (QueryChannel.techPending[i])
        {
            // Still there's tech channel list return pending
            return true;
        }
    }
    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function initializes/resets the QueryChannel data structure and sets its pending flags for
 * supported technologies as a preprataion for a brand new channel scan. Such flag setting should
 * be done before each technology is queried.
 */
//--------------------------------------------------------------------------------------------------
void DcsTechInitQueryChannelList
(
    void
)
{
    // Initialize/reset QueryChannel to prepare for a new channel query
    memset(&QueryChannel, 0, sizeof(QueryChannel));
    QueryChannel.techPending[LE_DCS_TECH_CELLULAR] = 1;
#ifdef LE_CONFIG_ENABLE_WIFI
    QueryChannel.techPending[LE_DCS_TECH_WIFI] = 1;
#endif
#ifdef LE_CONFIG_ENABLE_ETHERNET
    QueryChannel.techPending[LE_DCS_TECH_ETHERNET] = 1;
#endif
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to trigger the given technology in the argument to get its list of all available
 * channels
 *
 * @return

 *     - The function returns LE_OK upon a successful trigger; otherwise, some other
 *       le_result_t failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t dcsTech_GetChannelList
(
    le_dcs_Technology_t tech
)
{
    le_result_t ret;

    LE_DEBUG("Querying channel list from tech %d", tech);
    switch (tech)
    {
        case LE_DCS_TECH_UNKNOWN:
            // LE_DCS_TECH_UNKNOWN has enum value 0 which is used as a signal from le_dcs_GetList()
            // to indicate the start of a query; thus, reset QueryChannel as a preparation
            if (dcsTech_ChannelQueryIsPending(LE_DCS_TECH_MAX))
            {
                // Don't reset as collection is already in action
                return LE_OK;
            }
            DcsTechInitQueryChannelList();
            return LE_OK;
        case LE_DCS_TECH_CELLULAR:
            // For cellular the channel list query is a synchronous call. After the function
            // call below, the list would have been learned back and its pending flag reset
            ret = le_dcsCellular_GetChannelList();
            break;
#ifdef LE_CONFIG_ENABLE_WIFI
        case LE_DCS_TECH_WIFI:
            // For wifi the channel list query is an asynchronous call. After the function call
            // below, a wifi scan would have been triggered with no results yet available and its
            // pending flag still set, until wifi posts a notification about scan completion.
            ret = le_dcsWifi_GetChannelList();
            break;
#endif
#ifdef LE_CONFIG_ENABLE_ETHERNET
        case LE_DCS_TECH_ETHERNET:
            // For Ethernet the channel list query is a synchronous call. After the function
            // call below, the list would have been learned back and its pending flag reset
            ret = le_dcsEthernet_GetChannelList();
            break;
#endif
        default:
            LE_ERROR("Unsupported technology %d", tech);
            ret = LE_UNSUPPORTED;
    }

    if ((ret != LE_OK) && (ret != LE_DUPLICATE))
    {
        LE_WARN("Failed to trigger channel list collection for technology %d; error: %d",
                tech, ret);
        QueryChannel.techPending[tech] = 0;
    }
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for querying the network interface of the given channel specified in the 1st argument
 * after its technology type is retrieved
 *
 * @return
 *     - The retrieved network interface's name will be returned in the 2nd argument which allowed
 *       buffer length is specified in the 3rd argument that is to be observed strictly
 *     - The function returns LE_OK upon a successful retrieval; otherwise, some other le_result_t
 *       failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t dcsTech_GetNetInterface
(
    le_dcs_Technology_t tech,
    le_dcs_ChannelRef_t channelRef,
    char *intfName,
    int nameSize
)
{
    le_result_t ret;
    le_dcs_channelDb_t *channelDb;

    if ((tech == LE_DCS_TECH_UNKNOWN) || (tech >= LE_DCS_TECH_MAX))
    {
        LE_ERROR("Channel's technology type %s not supported", dcs_ConvertTechEnumToName(tech));
        return LE_UNSUPPORTED;
    }

    channelDb = dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Invalid channel reference %p for getting network interface", channelRef);
        return LE_FAULT;
    }

    intfName[0] = '\0';
    switch (tech)
    {
        case LE_DCS_TECH_CELLULAR:
            ret = le_dcsCellular_GetNetInterface(channelDb->techRef, intfName, nameSize);
            break;
#ifdef LE_CONFIG_ENABLE_WIFI
        case LE_DCS_TECH_WIFI:
            ret = le_dcsWifi_GetNetInterface(channelDb->techRef, intfName, nameSize);
            break;
#endif
#ifdef LE_CONFIG_ENABLE_ETHERNET
        case LE_DCS_TECH_ETHERNET:
            ret = le_dcsEthernet_GetNetInterface(channelDb->techRef, intfName, nameSize);
            break;
#endif
        default:
            LE_ERROR("Unsupported technology %d", tech);
            return LE_UNSUPPORTED;
    }
    if (LE_OK != ret)
    {
        LE_ERROR("Failed to get network interface of channel %s of technology %s",
                 channelDb->channelName,
                 dcs_ConvertTechEnumToName(tech));
    }
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for requesting cellular to start the given data channel in the 1st argument
 * after its technology type is retrieved
 *
 * @return
 *     - The function returns LE_OK or LE_DUPLICATE upon a successful start; otherwise, some other
 *       le_result_t failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t dcsTech_Start
(
    const char *channelName,
    le_dcs_Technology_t tech
)
{
    le_result_t ret;
    le_dcs_channelDb_t *channelDb;

    channelDb = dcs_GetChannelDbFromName(channelName, tech);
    if (!channelDb)
    {
        LE_ERROR("Channel %s isn't available", channelName);
        return LE_FAULT;
    }

    LE_INFO("Request to start channel %s of technology %s", channelName,
            dcs_ConvertTechEnumToName(tech));
    switch (tech)
    {
        case LE_DCS_TECH_CELLULAR:
            ret = le_dcsCellular_Start(channelDb->techRef);

            // Treat packet switch state down as temporary down.
            // Cover use case of dcsDaemon where it will retry the moment PS state is re-attached
            if (ret == LE_UNAVAILABLE)
            {
                LE_DEBUG("Cellular channel packet state down.");
                dcs_ChannelEventNotifier(channelDb->channelRef, LE_DCS_EVENT_TEMP_DOWN);
                return ret;
            }
            break;
#ifdef LE_CONFIG_ENABLE_WIFI
        case LE_DCS_TECH_WIFI:
            ret = le_dcsWifi_Start(channelDb->techRef);
            break;
#endif
#ifdef LE_CONFIG_ENABLE_ETHERNET
        case LE_DCS_TECH_ETHERNET:
            ret = le_dcsEthernet_Start(channelDb->techRef);
            break;
#endif
        default:
            LE_ERROR("Unsupported technology %d", tech);
            ret = LE_UNSUPPORTED;
    }

    if ((ret != LE_OK) && (ret != LE_DUPLICATE))
    {
        LE_ERROR("Failed to start channel %s; error: %d", channelName, ret);
        dcs_ChannelEventNotifier(channelDb->channelRef, LE_DCS_EVENT_DOWN);
    }
    else
    {
        LE_DEBUG("Succeeded to request starting channel %s", channelName);
    }
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for stopping the given data channel in the argument after its technology type
 * is retrieved
 *
 * @return
 *     - The function returns LE_OK upon a successful stop; otherwise, some other
 *       le_result_t failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t dcsTech_Stop
(
    const char *channelName,
    le_dcs_Technology_t tech
)
{
    le_result_t ret;
    le_dcs_channelDb_t *channelDb;

    channelDb = dcs_GetChannelDbFromName(channelName, tech);
    if (!channelDb)
    {
        LE_ERROR("Db for channel %s not found", channelName);
        return LE_FAULT;
    }

    LE_INFO("Request to stop channel %s of technology %s", channelName,
            dcs_ConvertTechEnumToName(tech));
    switch (tech)
    {
        case LE_DCS_TECH_CELLULAR:
            ret = le_dcsCellular_Stop(channelDb->techRef);
            break;
#ifdef LE_CONFIG_ENABLE_WIFI
        case LE_DCS_TECH_WIFI:
            ret = le_dcsWifi_Stop(channelDb->techRef);
            break;
#endif
#ifdef LE_CONFIG_ENABLE_ETHERNET
        case LE_DCS_TECH_ETHERNET:
            ret = le_dcsEthernet_Stop(channelDb->techRef);
            break;
#endif
        default:
            LE_ERROR("Unsupported technology %d", tech);
            ret = LE_UNSUPPORTED;
    }

    if ((ret != LE_OK) && (ret != LE_DUPLICATE))
    {
        LE_WARN("Failed to stop channel %s; error: %d", channelName, ret);
    }
    else
    {
        LE_DEBUG("Succeeded to stop channel %s", channelName);
    }
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for checking if the given channel's operational state is up or not
 *
 * @return
 *     - In the function's return value, the bool is returned to indicate whether the given
 *       channel's tech db is up or not
 */
//--------------------------------------------------------------------------------------------------
bool dcsTech_GetOpState
(
    le_dcs_channelDb_t *channelDb
)
{
    switch (channelDb->technology)
    {
        case LE_DCS_TECH_CELLULAR:
            return le_dcsCellular_GetOpState(channelDb->techRef);
#ifdef LE_CONFIG_ENABLE_WIFI
        case LE_DCS_TECH_WIFI:
            return le_dcsWifi_GetOpState(channelDb->techRef);
#endif
#ifdef LE_CONFIG_ENABLE_ETHERNET
        case LE_DCS_TECH_ETHERNET:
            return le_dcsEthernet_GetOpState(channelDb->techRef);
#endif
        default:
            LE_ERROR("Unsupported technology %s",
                     dcs_ConvertTechEnumToName(channelDb->technology));
    }
    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for initiating the given channel to retry after a channel failure. Within the
 * technology itself, the failure cause & code are retrievable.
 *
 * @return
 *     - LE_OK if the retry has been successfully triggered
 *     - LE_FAULT if it has failed, e.g. due to missing connection db
 *     - LE_OVERFLOW if there are more active connections than allowed
 *     - LE_DUPLICATE if the connection needs no retry as it is already up
 */
//--------------------------------------------------------------------------------------------------
le_result_t dcsTech_RetryChannel
(
    le_dcs_channelDb_t *channelDb
)
{
    switch (channelDb->technology)
    {
        case LE_DCS_TECH_CELLULAR:
            return le_dcsCellular_RetryConn(channelDb->techRef);
        case LE_DCS_TECH_WIFI:
        default:
            LE_ERROR("Unsupported technology %s",
                     dcs_ConvertTechEnumToName(channelDb->technology));
    }
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for querying the default GW address of the given connection specified in the 2nd
 * argument of the technology given in the 1st
 *
 * @return
 *     - The retrieved IPv4 default GW address will be returned in the 3rd argument which allowed
 *       buffer length is specified in the 4th argument. Similarly the 5th & 6th arguments for
 *       the retrieved IPv6 default GW address
 *     - The function returns:
 *           - LE_OK upon a successful retrieval
 *           - LE_UNSUPPORTED upon an unsupported technolgoy type given in the 1st argument
 *           - LE_FAULT otherwise
 */
//--------------------------------------------------------------------------------------------------
le_result_t dcsTech_GetDefaultGWAddress
(
    le_dcs_Technology_t tech,    ///< [IN] technology type of the connection
    le_dcs_ChannelRef_t channelRef,  ///< [IN] object reference of the channel
    char *v4GwAddrPtr,           ///< [OUT] IPv4 default GW addresses to be installed
    size_t v4GwAddrSize,         ///< [IN] size of each of the IPv4 default GW addresses
    char *v6GwAddrPtr,           ///< [OUT] IPv6 default GW addresses to be installed
    size_t v6GwAddrSize          ///< [IN] size of each of the IPv6 default GW addresses
)
{
    char intf[LE_DCS_INTERFACE_NAME_MAX_LEN] = {0};
    le_dcs_channelDb_t *channelDb = dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Invalid channel reference %p", channelRef);
        return LE_FAULT;
    }

    switch (tech)
    {
        case LE_DCS_TECH_CELLULAR:
            return le_dcsCellular_GetDefaultGWAddress(channelDb->techRef, v4GwAddrPtr, v4GwAddrSize,
                                                      v6GwAddrPtr, v6GwAddrSize);
        case LE_DCS_TECH_WIFI:
        case LE_DCS_TECH_ETHERNET:
            if (LE_OK != dcsTech_GetNetInterface(tech, channelRef, intf,
                                                    LE_DCS_INTERFACE_NAME_MAX_LEN))
            {
                LE_ERROR("Failed to get network interface for channel %s of technology %s to set "
                         "default GW", channelDb->channelName,
                         dcs_ConvertTechEnumToName(tech));
                return LE_FAULT;
            }
            // The last argument is 1 since only 1 address is to be retrieved per IP type, which is
            // also the # of v4DnsAddrs and v6DnsAddrs in the input arguments
            return net_GetLeaseAddresses(intf, LE_NET_DEFAULT_GATEWAY_ADDRESS,
                                         v4GwAddrPtr, v4GwAddrSize, v6GwAddrPtr, v6GwAddrSize,
                                         1);
        default:
            LE_ERROR("Unsupported technology %s", dcs_ConvertTechEnumToName(tech));
            return LE_UNSUPPORTED;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for querying the DNS addresses of the given connection specified in the 2nd
 * argument of the technology given in the 1st. For each of the IP version type, up to 2 DNS
 * addresses can be returned. Thus, each of the 2 input arrays v4DnsAddrsPtr & v6DnsAddrsPtr
 * consists of 2 address elements of the same max length specified by v4DnsAddrSize or v6DnsAddrSize,
 * i.e. v4DnsAddrsPtr & v6DnsAddrsPtr are char [2][DnsAddrSize].
 *
 * @return
 *     - The retrieved IPv4 DNS address(es) will be returned in the 3rd & the IPv6 ones in 5th
 *       arguments which allowed buffer lengths are specified in the 4th & 6th arguments
 *       respectively. It's up to 2 addresses to be returned per IP type.
 *     - The function returns:
 *           - LE_OK upon successful retrievals
 *           - LE_UNSUPPORTED upon an unsupported technolgoy type given in the 1st argument
 *           - LE_FAULT otherwise
 */
//--------------------------------------------------------------------------------------------------
le_result_t dcsTech_GetDNSAddresses
(
    le_dcs_Technology_t tech,    ///< [IN] technology type of the connection
    le_dcs_ChannelRef_t channelRef,  ///< [IN] object reference of the channel
    char *v4DnsAddrs,            ///< [OUT] 2 IPv4 DNS addresses, each of size v4DnsAddrSize
    size_t v4DnsAddrSize,        ///< [IN] size of each of the IPv4 DNS addresses to be installed
    char *v6DnsAddrs,            ///< [OUT] 2 IPv6 DNS addresses, each of size v6DnsAddrSize
    size_t v6DnsAddrSize         ///< [IN] size of each of the IPv6 DNS addresses to be installed
)
{
    char intf[LE_DCS_INTERFACE_NAME_MAX_LEN] = {0};
    le_dcs_channelDb_t *channelDb = dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Invalid channel reference %p", channelRef);
        return LE_FAULT;
    }

    switch (tech)
    {
        case LE_DCS_TECH_CELLULAR:
            return le_dcsCellular_GetDNSAddrs(channelDb->techRef, v4DnsAddrs, v4DnsAddrSize,
                                              v6DnsAddrs, v6DnsAddrSize);
        case LE_DCS_TECH_WIFI:
        case LE_DCS_TECH_ETHERNET:
            if (LE_OK != dcsTech_GetNetInterface(tech, channelRef, intf,
                                                    LE_DCS_INTERFACE_NAME_MAX_LEN))
            {
                LE_ERROR("Failed to get network interface for channel %s of technology %s to set "
                         "DNS addresses", channelDb->channelName,
                         dcs_ConvertTechEnumToName(tech));
                return LE_FAULT;
            }
            // The last argument is 2 since 2 DNS addresses are to be retrieved per IP type, which
            // is also the array size of v4DnsAddrs and v6DnsAddrs in the input arguments
            return net_GetLeaseAddresses(intf, LE_NET_DNS_SERVER_ADDRESS,
                                         (char *)v4DnsAddrs, v4DnsAddrSize,
                                         (char *)v6DnsAddrs, v6DnsAddrSize, 2);
        default:
            LE_ERROR("Unsupported technology %s", dcs_ConvertTechEnumToName(tech));
            return LE_UNSUPPORTED;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for checking with the technology upfront if its given connection specified in the 2nd
 * argument can be allowed to start
 *
 * @return
 *     - LE_OK if it's allowed
 *     - LE_UNSUPPORTED if the given technology isn't supported or available yet
 *     - LE_DUPLICATE if the given channel is already connected
 *     - LE_NOT_PERMITTED if the technology doesn't allow this channel to be connected
 */
//--------------------------------------------------------------------------------------------------
le_result_t dcsTech_AllowChannelStart
(
    le_dcs_Technology_t tech,
    const char *channelName
)
{
    le_dcs_channelDb_t *channelDb = dcs_GetChannelDbFromName(channelName, tech);
    if (!channelDb)
    {
        LE_WARN("Channel db for le_dcs not found for channel name %s of technology %s", channelName,
                dcs_ConvertTechEnumToName(tech));
        return LE_FAULT;
    }

    switch (tech)
    {
        case LE_DCS_TECH_CELLULAR:
            return le_dcsCellular_AllowChannelStart(channelDb->techRef);
#ifdef LE_CONFIG_ENABLE_WIFI
        case LE_DCS_TECH_WIFI:
            return le_dcsWifi_AllowChannelStart(channelDb->techRef);
#endif
#ifdef LE_CONFIG_ENABLE_ETHERNET
        case LE_DCS_TECH_ETHERNET:
            return le_dcsEthernet_AllowChannelStart(channelDb->techRef);
#endif
        default:
            LE_ERROR("Unsupported technology %s", dcs_ConvertTechEnumToName(tech));
            return LE_UNSUPPORTED;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for traversing each channel included on the given channel list of the given list size
 * in the function arguments and check if a channelDb has not been created for it. If so,
 * call dcs_CreateChannelDb() to create one for it.
 */
//--------------------------------------------------------------------------------------------------
static void DcsTechUpdateChannelDbList
(
    le_dcs_Technology_t tech,
    le_dcs_ChannelInfo_t *channelList,
    size_t listLen
)
{
    le_dcs_channelDb_t *channelDb;
    le_dcs_ChannelRef_t channelRef;
    uint16_t i;

    // Create for any new channel its dbs & its reference into the struct to be returned
    for (i = 0; i < listLen; i++)
    {
        channelDb = dcs_GetChannelDbFromName(channelList[i].name, tech);
        if (!channelDb)
        {
            // It's a newly learned channel; create its dbs
            channelRef = dcs_CreateChannelDb(tech, channelList[i].name);
            if (!channelRef)
            {
                LE_ERROR("Failed to create dbs for new channel %s of technology %d",
                         channelList[i].name, tech);
                memset(&channelList[i], 0x0, sizeof(le_dcs_ChannelInfo_t));
                continue;
            }
        }
        else
        {
            channelRef = channelDb->channelRef;
        }
        channelList[i].ref = channelRef;
        // Retrieve each channel's admin state here
        if (LE_OK != dcs_GetAdminState(channelList[i].ref, &channelList[i].state))
        {
            LE_ERROR("Failed to update admin state of channel %s of technology %d",
                     channelList[i].name, tech);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for checking if all channel lists have been collected and it's time to send a channel
 * list query notification back to apps. If so, call le_dcs_ChannelQueryNotifier() to send the
 * collected channel list back to the interested apps via their handlers
 */
//--------------------------------------------------------------------------------------------------
static void DcsTechPostChannelList
(
    void
)
{
    if (dcsTech_ChannelQueryIsPending(LE_DCS_TECH_MAX))
    {
        LE_DEBUG("Not done collecting available channel lists to post query results");
        return;
    }

    // No more tech channel list return pending; notify apps now
    LE_INFO("Posting collected channel list to apps of size %d", QueryChannel.listSize);
    dcs_ChannelQueryNotifier(LE_OK, QueryChannel.list, QueryChannel.listSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for collecting channel list results of a query from a given technology
 */
//--------------------------------------------------------------------------------------------------
void dcsTech_CollectChannelQueryResults
(
    le_dcs_Technology_t technology,
    le_result_t result,
    le_dcs_ChannelInfo_t *channelList,
    size_t listSize
)
{
    uint16_t i;

    LE_INFO("Query channel list results collected from technology %d, retcode %d, list size %d",
            technology, result, (int)listSize);

    if ((technology >= LE_DCS_TECH_MAX) || (technology == LE_DCS_TECH_UNKNOWN))
    {
        LE_ERROR("Invalid technology input for channel list collection");
        return;
    }

    if ((result != LE_OK) || !channelList || (listSize == 0))
    {
        // No need to archive list
        LE_DEBUG("Query channel result collector need not archive results");
        QueryChannel.techPending[technology] = 0;
        DcsTechPostChannelList();
        return;
    }

    DcsTechUpdateChannelDbList(technology, channelList, listSize);

    // Archive list
    if ((QueryChannel.listSize + listSize) > LE_DCS_CHANNEL_LIST_QUERY_MAX)
    {
        listSize = LE_DCS_CHANNEL_LIST_QUERY_MAX - QueryChannel.listSize;
        LE_DEBUG("Query channel list maxed out; collected list trimmed to size %u",
                 (int)listSize);
    }
    for (i=0; i<listSize; i++)
    {
        memcpy(&QueryChannel.list[QueryChannel.listSize], &channelList[i],
               sizeof(le_dcs_ChannelInfo_t));
        QueryChannel.listSize++;
    }
    QueryChannel.techPending[technology] = 0;

    DcsTechPostChannelList();
}

/* vim:et:sw=4:ts=4:tw=79: */
/*****************************************************************************
 * File:            fm10000_api_an.c
 * Creation Date:   April 5, 2014
 * Description:     Autonegotiation for SGMII, Clause 37, and Clause 73.
 *
 * Copyright (c) 2011 - 2015, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Intel Corporation nor the names of its contributors
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include <fm_sdk_fm10000_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/



/*****************************************************************************
 * Local Function Prototypes
 *****************************************************************************/
static fm_status NotifyClause73Events(fm_int sw, fm_int port, fm_uint32 anIp);
static fm_status NotifyClause37Events(fm_int sw, fm_int port, fm_uint32 anIp);
static fm_status IsAn25GConfiguredInNextPage(fm_int         sw,
                                             fm_int         port,
                                             fm_anNextPages nextPages,
                                             fm_bool        *is25GConfigured);


/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************/
/** NotifyClause73Events
 * \ingroup intPort
 *
 * \desc            Local helper function that scans the list of possible
 *                  Clause 73 interrupt sources and sends the corresponding
 *                  events to the AN state machine
 *
 * \param[in]       sw is the ID of the switch on which to operate
 * 
 * \param[in]       port is the ID of the port on which to operate
 *
 * \param[in]       anIp is the AN Interrupt Pending mask 
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status NotifyClause73Events( fm_int sw, fm_int port, fm_uint32 anIp )
{
    fm_smEventInfo  eventInfo;
    fm10000_port   *portExt;
    fm_status       status;

    eventInfo.smType = FM10000_CLAUSE73_AN_STATE_MACHINE;
    eventInfo.lock   = FM_GET_STATE_LOCK( sw );
    eventInfo.dontSaveRecord = FALSE;
    portExt = GET_PORT_EXT( sw, port );

    status = FM_OK;

    if ( FM_GET_BIT( anIp, FM10000_AN_IP, An73AbilityDetect ) )
    {
        /* Ability Detect */
        eventInfo.eventId = FM10000_AN_EVENT_ABILITY_DETECT_IND;
        portExt->eventInfo.regLockTaken = FALSE;
        status = fmNotifyStateMachineEvent( portExt->anSmHandle,
                                            &eventInfo,
                                            &portExt->eventInfo,
                                            &port );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, port, status );
    }

    if ( FM_GET_BIT( anIp, FM10000_AN_IP, An73AcknowledgeDetect ) )
    {
        /* Acknowledge Detect */
        eventInfo.eventId = FM10000_AN_EVENT_ACK_DETECT_IND;
        portExt->eventInfo.regLockTaken = FALSE;
        status = fmNotifyStateMachineEvent( portExt->anSmHandle,
                                            &eventInfo,
                                            &portExt->eventInfo,
                                            &port );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, 
                                port, 
                                status );
    }

    if ( FM_GET_BIT(anIp, FM10000_AN_IP, An73CompleteAcknowledge) )
    {
        /* Complete acknowledge */
        eventInfo.eventId = FM10000_AN_EVENT_COMPLETE_ACK_IND;
        portExt->eventInfo.regLockTaken = FALSE;
        status = fmNotifyStateMachineEvent( portExt->anSmHandle,
                                            &eventInfo,
                                            &portExt->eventInfo,
                                            &port );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, 
                                port, 
                                status );
    }

    if ( FM_GET_BIT( anIp, FM10000_AN_IP, An73NextPageWait ) )
    {
        /* Next Page Wait */
        eventInfo.eventId = FM10000_AN_EVENT_NEXT_PAGE_WAIT_IND;
        portExt->eventInfo.regLockTaken = FALSE;
        status = fmNotifyStateMachineEvent( portExt->anSmHandle,
                                            &eventInfo,
                                            &portExt->eventInfo,
                                            &port );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, 
                                port, 
                                status );
    }

    if ( FM_GET_BIT( anIp, FM10000_AN_IP, An73AnGoodCheck ) )
    {
        /* Good Check */
        eventInfo.eventId = FM10000_AN_EVENT_GOOD_CHECK_IND;
        portExt->eventInfo.regLockTaken = FALSE;
        status = fmNotifyStateMachineEvent( portExt->anSmHandle,
                                            &eventInfo,
                                            &portExt->eventInfo,
                                            &port );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, 
                                port, 
                                status );
    }

    if ( FM_GET_BIT( anIp, FM10000_AN_IP, An73AnGood ) )
    {
        /* Good */
        eventInfo.eventId = FM10000_AN_EVENT_GOOD_IND;
        portExt->eventInfo.regLockTaken = FALSE;
        status = fmNotifyStateMachineEvent( portExt->anSmHandle,
                                            &eventInfo,
                                            &portExt->eventInfo,
                                            &port );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, 
                                port, 
                                status );
    }

    if ( FM_GET_BIT( anIp, FM10000_AN_IP, An73TransmitDisable ) )
    {
        /* Transmit Disable */
        eventInfo.eventId = FM10000_AN_EVENT_TRANSMIT_DISABLE_IND;
        portExt->eventInfo.regLockTaken = FALSE;
        status = fmNotifyStateMachineEvent( portExt->anSmHandle,
                                            &eventInfo,
                                            &portExt->eventInfo,
                                            &port );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, port, status );
    }

ABORT:
    return status;

}   /* end NotifyClause73Events */



/*****************************************************************************/
/** NotifyClause37Events
 * \ingroup intPort
 *
 * \desc            Local helper function that scans the list of possible
 *                  Clause 37 interrupt sources and sends the corresponding
 *                  events to the AN state machine
 *
 * \param[in]       sw is the ID of the switch on which to operate
 * 
 * \param[in]       port is the ID of the port on which to operate
 *
 * \param[in]       anIp is the AN Interrupt Pending mask 
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status NotifyClause37Events( fm_int sw, fm_int port, fm_uint32 anIp )
{
    fm_smEventInfo  eventInfo;
    fm10000_port   *portExt;
    fm_status       status;

    eventInfo.smType = FM10000_CLAUSE37_AN_STATE_MACHINE;
    eventInfo.lock   = FM_GET_STATE_LOCK( sw );
    eventInfo.dontSaveRecord = FALSE;
    portExt = GET_PORT_EXT( sw, port );

    status = FM_OK;

    if ( FM_GET_BIT( anIp, FM10000_AN_IP, An37AnEnable ) )
    {
        /* AN Enable */
        eventInfo.eventId = FM10000_AN_EVENT_ENABLE_IND;
        portExt->eventInfo.regLockTaken = FALSE;
        status = fmNotifyStateMachineEvent( portExt->anSmHandle,
                                            &eventInfo,
                                            &portExt->eventInfo,
                                            &port );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, 
                                port, 
                                status );
    }

    if ( FM_GET_BIT( anIp, FM10000_AN_IP, An37AnRestart ) )
    {
        /* AN Restart  */
        eventInfo.eventId = FM10000_AN_EVENT_RESTART_IND;
        portExt->eventInfo.regLockTaken = FALSE;
        status = fmNotifyStateMachineEvent( portExt->anSmHandle,
                                            &eventInfo,
                                            &portExt->eventInfo,
                                            &port );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, 
                                port, 
                                status );
    }

    if ( FM_GET_BIT( anIp, FM10000_AN_IP, An37AnDisableLinkOk ) )
    {
        /* Disable Link OK */
        eventInfo.eventId = FM10000_AN_EVENT_DISABLE_LINK_OK_IND;
        portExt->eventInfo.regLockTaken = FALSE;
        status = fmNotifyStateMachineEvent( portExt->anSmHandle,
                                            &eventInfo,
                                            &portExt->eventInfo,
                                            &port );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, 
                                port, 
                                status );
    }
    if ( FM_GET_BIT( anIp, FM10000_AN_IP, An37AbilityDetect ) )
    {
        /* Ability Detect */
        eventInfo.eventId = FM10000_AN_EVENT_ABILITY_DETECT_IND;
        portExt->eventInfo.regLockTaken = FALSE;
        status = fmNotifyStateMachineEvent( portExt->anSmHandle,
                                            &eventInfo,
                                            &portExt->eventInfo,
                                            &port );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, 
                                port, 
                                status );
    }

    if ( FM_GET_BIT( anIp, FM10000_AN_IP, An37CompleteAcknowledge ) ) 
    {
        /* Complete Acknowledge */
        eventInfo.eventId = FM10000_AN_EVENT_COMPLETE_ACK_IND;
        portExt->eventInfo.regLockTaken = FALSE;
        status = fmNotifyStateMachineEvent( portExt->anSmHandle,
                                            &eventInfo,
                                            &portExt->eventInfo,
                                            &port );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, 
                                port, 
                                status );
    }

    if ( FM_GET_BIT( anIp, FM10000_AN_IP, An37NextPageWait ) )
    {
        /* Next Page Wait */
        eventInfo.eventId = FM10000_AN_EVENT_GOOD_CHECK_IND;
        portExt->eventInfo.regLockTaken = FALSE;
        status = fmNotifyStateMachineEvent( portExt->anSmHandle,
                                            &eventInfo,
                                            &portExt->eventInfo,
                                            &port );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, 
                                port, 
                                status );
    }

    if ( FM_GET_BIT( anIp, FM10000_AN_IP, An37IdleDetect ) )
    {
        /* Idle Detect */
        eventInfo.eventId = FM10000_AN_EVENT_IDLE_DETECT_IND;
        portExt->eventInfo.regLockTaken = FALSE;
        status = fmNotifyStateMachineEvent( portExt->anSmHandle,
                                            &eventInfo,
                                            &portExt->eventInfo,
                                            &port );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, 
                                port, 
                                status );
    }

    if ( FM_GET_BIT( anIp, FM10000_AN_IP, An37LinkOk ) )
    {
        /* Link OK */
        eventInfo.eventId = FM10000_AN_EVENT_LINK_OK_IND;
        portExt->eventInfo.regLockTaken = FALSE;
        status = fmNotifyStateMachineEvent( portExt->anSmHandle,
                                            &eventInfo,
                                            &portExt->eventInfo,
                                            &port );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, 
                                port, 
                                status );
    }

ABORT:
    return status;

}   /* end NotifyClause37Events */



/*****************************************************************************/
/** IsAn25GConfiguredInNextPage
 * \ingroup intPort
 *
 * \desc            Helper function to determine if 25G is 
 *                  configured in nextPages.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the ID of the port to operate on.
 *
 * \param[in]       nextPages is the nextPages in which supported 25G is checked.
 *
 * \param[out]      is25GConfigured is where the result is returned.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
static fm_status IsAn25GConfiguredInNextPage(fm_int         sw,
                                             fm_int         port,
                                             fm_anNextPages nextPages,
                                             fm_bool        *is25GConfigured)
{
    fm_status status;
    fm_uint   extTechAbIndex;
    fm_uint64 page;

    *is25GConfigured = FALSE;
    status = fm10000AnGetNextPageExtTechAbilityIndex(sw,
                                                     port,
                                                     nextPages.nextPages,
                                                     nextPages.numPages,
                                                     &extTechAbIndex,
                                                     "Tx");

    if (status == FM_OK)
    {
        page = nextPages.nextPages[extTechAbIndex];
        if ( FM_GET_UNNAMED_BIT64(page, 21) ||
             FM_GET_UNNAMED_BIT64(page, 20) )
        {
            *is25GConfigured = TRUE;    
        }
    }
    else if (status == FM_ERR_NOT_FOUND)
    {
        *is25GConfigured = FALSE;
        status = FM_OK;
    }

    return status;

}   /* end IsAn25GConfiguredInNextPage */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000IsPortAutonegReady
 * \ingroup intPort
 *
 * \desc            Function to verify if a port is properly configured to 
 *                  start Auto-negotiation
 * 
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the logical port to configure.
 * 
 * \param[in]       ethMode is the current or desired ethernet interface mode
 * 
 * \param[in]       anMode is the current or desired autoneg mode 
 *
 * \param[out]      ready is a pointer to a caller-provided boolean variable
 *                  set to TRUE by this function if this port is ready for
 *                  Auto-negotiation
 * 
 * \param[out]      smType is a pointer to a caller-provided variable set to
 *                  the AN State Machine Type associated to the current
 *                  Autonegotiation mode
 *
 * \return          FM_OK if successful.
 * 
 * \return          FM_ERR_INVALID_ARGUMENT if the ready pointer is NULL
 * 
 * \return          FM_ERR_UNSUPPORTED if this a port type for which
 *                  Autonegotiation isn't supported 
 *****************************************************************************/
fm_status fm10000IsPortAutonegReady( fm_int     sw, 
                                     fm_int     port, 
                                     fm_ethMode ethMode,
                                     fm_uint32  anMode,
                                     fm_bool   *ready,
                                     fm_int    *smType )
{
    fm10000_port     *portExt;
    fm_port          *portPtr;
    fm_status         status;

    if ( ready == NULL || smType == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    portExt = GET_PORT_EXT( sw, port );
    portPtr = GET_PORT_PTR( sw, port );

    status  = FM_OK;
    *ready  = FALSE;

    /* ethernet ports only */
    if ( ( (portPtr->portType == FM_PORT_TYPE_PHYSICAL)
          || ( (portPtr->portType == FM_PORT_TYPE_CPU) && (port != 0) ) )
         && (portExt->ring == FM10000_SERDES_RING_EPL) )
    {
        *smType = portExt->anSmType;

        /* Clause 73 */
        if ( anMode == FM_PORT_AUTONEG_CLAUSE_73 )
        {
            *smType = FM10000_CLAUSE73_AN_STATE_MACHINE;
            if ( ethMode == FM_ETH_MODE_AN_73 )
            {
                *ready  = TRUE;
            }
        }

        /* Clause 37 or SGMII */
        if ( anMode == FM_PORT_AUTONEG_CLAUSE_37 ||
             anMode == FM_PORT_AUTONEG_SGMII )
        {
            *smType = FM10000_CLAUSE37_AN_STATE_MACHINE;
            if ( ethMode == FM_ETH_MODE_1000BASE_X || 
                ethMode == FM_ETH_MODE_SGMII )
            {
                *ready  = TRUE;
            }
        }
    }
    else
    {
        status  = FM_ERR_INVALID_PORT;
        *smType = FM_SMTYPE_UNSPECIFIED;
    }

    return status;

}   /* end fm10000IsPortAutonegReady */


/*****************************************************************************/
/** fm10000An73SetLinkInhibitTimer
 * \ingroup intPort
 *
 * \desc            Set link fail inhibit timer.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[in]       timeout is the time value in milliseconds to configure.
 *                  NOTE: The saved timeout might be different than
 *                  then configured timeout to reflect values supported in
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000An73SetLinkInhibitTimer( fm_int  sw,
                                          fm_int  port,
                                          fm_uint timeout )
{
    fm10000_portAttr *portAttrExt;
    fm_uint           linkTimeoutMax;

    portAttrExt = GET_FM10000_PORT_ATTR(sw, port);

    /* Use the default */
    if (timeout == 0)
    {
        portAttrExt->autoNegLinkInhbTimer = LINK_INHIBIT_TIMER_MILLISEC;
        FM_LOG_EXIT_V2( FM_LOG_CAT_PORT_AUTONEG, port, FM_OK );
    }

    /* Timescale is equal 5 (granularity => 1ms) for timeout < 512
     * or 6 (granularity => 10ms) for timeout >= 512 and <= 1023, so values
     * from 512 to 1023 will be rounded down to the closest multiple of 10.
     * By default a valid range is from 1 to 511, but it is possible to extend
     * it to <1..1023> by setting attribute api.an.timerAllowOutSpec */
    if (GET_PROPERTY()->anTimerAllowOutSpec)
    {
        linkTimeoutMax = FM10000_AN73_LINK_FAIL_INHIBIT_TIMEOUT_DEBUG;
    }
    else
    {
        linkTimeoutMax = FM10000_AN73_LINK_FAIL_INHIBIT_TIMEOUT_MAX;
    }
    if (timeout > linkTimeoutMax || timeout < 1)
    {
        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                         port,
                         "Invalid configuration: "
                         "LinkTimerTimeout=%d\n",
                         timeout );
        FM_LOG_EXIT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, 
                               port,
                               FM_ERR_INVALID_ARGUMENT );
    }

    portAttrExt->autoNegLinkInhbTimer = timeout;

    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT_AUTONEG, port, FM_OK);

}   /* end fm10000An73SetLinkInhibitTimer */



/*****************************************************************************/
/** fm10000An73SetLinkInhibitTimerKx
 * \ingroup intPort
 *
 * \desc            Set link fail inhibit timer for KX or KX4.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[in]       timeout is the time value in milliseconds to configure.
 *                  NOTE: The saved timeout might be different than
 *                  then configured timeout to reflect values supported in
 *                  hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000An73SetLinkInhibitTimerKx( fm_int  sw,
                                            fm_int  port,
                                            fm_uint timeout )
{
    fm10000_portAttr *portAttrExt;
    fm_uint           linkTimeoutMax;

    portAttrExt = GET_FM10000_PORT_ATTR(sw, port);

    /* Use the default */
    if (timeout == 0)
    {
        portAttrExt->autoNegLinkInhbTimerKx = LINK_INHIBIT_TIMER_MILLISEC_KX;

        FM_LOG_EXIT_V2( FM_LOG_CAT_PORT_AUTONEG, port, FM_OK );
    }

    /* Timescale is equal 5 (granularity => 1ms) for timeout < 512
     * or 6 (granularity => 10ms) for timeout >= 512 and <= 1023, so values
     * from 512 to 1023 will be rounded down to the closest multiple of 10.
     * By default a valid range is from 1 to 511, but it is possible to extend
     * it to <1..1023> by setting attribute api.an.timerAllowOutSpec */
    if (GET_PROPERTY()->anTimerAllowOutSpec)
    {
        linkTimeoutMax = FM10000_AN73_LINK_FAIL_INHIBIT_TIMEOUT_DEBUG;
    }
    else
    {
        linkTimeoutMax = FM10000_AN73_LINK_FAIL_INHIBIT_TIMEOUT_MAX;
    }
    if (timeout > linkTimeoutMax || timeout < 1)
    {
        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                         port,
                         "Invalid configuration: "
                         "LinkTimerTimeout=%d\n",
                         timeout );
        FM_LOG_EXIT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, 
                               port,
                               FM_ERR_INVALID_ARGUMENT );
    }

    portAttrExt->autoNegLinkInhbTimerKx = timeout;

    FM_LOG_EXIT_V2(FM_LOG_CAT_PORT_AUTONEG, port, FM_OK);

}   /* end fm10000An73SetLinkInhibitTimerKx */


/*****************************************************************************/
/** fm10000An73SetIgnoreNonce
 * \ingroup intPort
 *
 * \desc            Tell the Clause 73 Autoneg engine whether to ignore the
 *                  NONCE field
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[in]       ignoreNonce set to FM_ENABLED to ignore the NONCE field,
 *                  FM_DISABLED otherwise 
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000An73SetIgnoreNonce( fm_int  sw,
                                     fm_int  port,
                                     fm_bool ignoreNonce )
{
    fm_switch        *switchPtr;
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;
    fm_uint32         addr;
    fm_uint32         anCfg;
    fm_status         err;
    fm_int            epl;
    fm_int            lane;
    fm_bool           regLockTaken = FALSE;

    switchPtr   = GET_SWITCH_PTR(sw);
    portExt     = GET_PORT_EXT(sw, port);
    portAttrExt = GET_FM10000_PORT_ATTR(sw, port);

    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                     port, 
                     "sw=%d port=%d, IgnoreNonce=%s\n",
                     sw,
                     port,
                     ignoreNonce ? "TRUE":"FALSE" );

    /* determine the address of the AN_73_CFG register */
    epl = portExt->endpoint.epl;
    lane = portExt->nativeLaneExt->physLane;
    addr = FM10000_AN_73_CFG(epl, lane);
   
    /* read-modify-write the AN_73_CFG register */
    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    /* read */
    err = switchPtr->ReadUINT32(sw, addr, &anCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, err);

    /* modify */
    FM_SET_BIT(anCfg, FM10000_AN_73_CFG, IgnoreNonceMatch, ignoreNonce);

    /* write */
    err = switchPtr->WriteUINT32(sw, addr, anCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, err);

    portAttrExt->autoNegIgnoreNonce = ignoreNonce;

ABORT:

    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    
    return err;

}   /* end fm10000AnSet73IgnoreNonce */


/*****************************************************************************/
/** fm10000AnGetTimeScale
 * \ingroup intPort
 *
 * \desc            Calculate the hardware timescale and timeout for a given
 *                  timeout value in micro seconds.
 *
 *
 * \param[in]       timeoutUsec is the input timeout value in micro seconds.
 *
 * \param[in]       timeoutMax is the max timeout the hardware supports.
 *
 * \param[in]       timeScale points to caller-allocated storage where this
 *                  function should place the timeScale value
 *
 * \param[in]       timeout points to caller-allocated storage where this
 *                  function should place the timeout value
 *
 * \return          timeout value configured in hardware.
 *****************************************************************************/
fm_uint fm10000AnGetTimeScale( fm_uint  timeoutUsec,
                               fm_uint  timeoutMax,
                               fm_uint *timeScale,
                               fm_uint *timeout )
{
    fm_uint ts;

    ts = 1;

    for (*timeScale = 2 ; *timeScale <= 7 ; (*timeScale)++)
    {
        *timeout = timeoutUsec / ts;

        /* ts = pow(10, (*timeScale) - 1) */
        ts = ts * 10;

        if (*timeout < timeoutMax)
        {
            return ts/10*(*timeout);
        }
    }

    return ts/10*(*timeout);

}   /* end  AnGetTimeScale */



/*****************************************************************************/
/** fm10000AnSendConfigEvent
 * \ingroup intPort
 *
 * \desc            Helper function to format and send a AN_CONFIG_REQ event to
 *                  a port state machine
 *
 * \param[in]       sw is the ID of the switch to operate on
 * 
 * \param[in]       port is the ID of the port to operate on
 * 
 * \param[in]       eventId is the ID of the event to be sent (DISABLE/CONFIG)
 * 
 * \param[in]       mode is the autonegotiation mode
 *                  
 * \param[in]       basepage is the autonegotiation base page
 * 
 * \param[in]       nextPages is the set of autonegotiation next pages
 *
 * \return          FM_OK if successful
 *
 * \return          FM_ERR_STATE_MACHINE_HANDLE if the port doesn't have a
 *                  valid state machine handle associated to it
 * 
 * \return          FM_ERR_STATE_MACHINE_TYPE  if the port isn't currently
 *                  bound to a valid state transition table type
 *
 *****************************************************************************/
fm_status fm10000AnSendConfigEvent( fm_int          sw,
                                    fm_int          port, 
                                    fm_int          eventId,
                                    fm_uint32       mode,
                                    fm_uint64       basepage,
                                    fm_anNextPages  nextPages )
{
    fm_smEventInfo  eventInfo;
    fm10000_port   *portExt;

    portExt = GET_PORT_EXT( sw, port );

    /* fill out the generic evnet structure */
    eventInfo.smType  = portExt->smType;
    eventInfo.eventId = eventId;
    eventInfo.lock    = FM_GET_STATE_LOCK( sw );
    eventInfo.dontSaveRecord = FALSE;

    /* fill out the event-specific info structure. In this case
       since there was no change in the autoneg configuration
       we simply copy the info from the portAttr structure */
    portExt->eventInfo.info.anConfig.autoNegMode      = mode;
    portExt->eventInfo.info.anConfig.autoNegBasePage  = basepage;
    portExt->eventInfo.info.anConfig.autoNegNextPages = nextPages;
    portExt->eventInfo.regLockTaken                   = FALSE;

    /* we're ready to go, send it */
    return fmNotifyStateMachineEvent( portExt->smHandle,  
                                      &eventInfo,         
                                      &portExt->eventInfo,
                                      &port );            

}   /* end fm10000AnSendConfigEvent */



/*****************************************************************************/
/** fm10000AnRestartOnNewConfig
 * \ingroup intPort
 *
 * \desc            Helper function to format and send a AN_CONFIG_REQ event to
 *                  a port state machine
 *
 * \param[in]       sw is the ID of the switch to operate on
 * 
 * \param[in]       port is the ID of the port to operate on
 * 
 * \param[in]       ethMode is the ethernet interface mode
 * 
 * \param[in]       anMode is the autonegotiation mode
 *                  
 * \param[in]       basepage is the autonegotiation base page
 * 
 * \param[in]       nextPages is the set of autonegotiation next pages
 *
 * \return          FM_OK if successful
 *
 * \return          FM_ERR_STATE_MACHINE_HANDLE if the port doesn't have a
 *                  valid state machine handle associated to it
 * 
 * \return          FM_ERR_STATE_MACHINE_TYPE  if the port isn't currently
 *                  bound to a valid state transition table type
 *****************************************************************************/
fm_status fm10000AnRestartOnNewConfig( fm_int          sw,
                                       fm_int          port, 
                                       fm_ethMode      ethMode,
                                       fm_uint32       anMode,
                                       fm_uint64       basepage,
                                       fm_anNextPages  nextPages )
{
    fm_status     status;
    fm10000_port *portExt;
    fm_portAttr  *portAttr;
    fm_int        newAnSmType;
    fm_bool       anReady;

    status = fm10000IsPortAutonegReady( sw, 
                                        port, 
                                        ethMode,
                                        anMode,
                                        &anReady, 
                                        &newAnSmType );
    if ( status == FM_OK && anReady == TRUE )
    {
        portExt = GET_PORT_EXT( sw, port );

        /* Do we need to switch state transition table on the fly? */
        if ( newAnSmType != portExt->anSmType )
        {
            /* It may be, stop the current state machine if any */
            if ( portExt->anSmType != FM_SMTYPE_UNSPECIFIED )
            {

                portAttr = GET_PORT_ATTR( sw, port );
                fm10000AnSendConfigEvent( sw,                              
                                          port,                            
                                          FM10000_PORT_EVENT_AN_DISABLE_REQ,
                                          portAttr->autoNegMode,
                                          portAttr->autoNegBasePage,       
                                          portAttr->autoNegNextPages );    

                fmStopStateMachine( portExt->anSmHandle );
            }

            /* start the new state machine */
            status = fmStartStateMachine( portExt->anSmHandle,
                                          newAnSmType,
                                          FM10000_AN_STATE_DISABLED );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

            portExt->anSmType = newAnSmType;

        }

        /* change the AN interrupt mask */
        if ( anMode == FM_PORT_AUTONEG_CLAUSE_73 )
        {
            portExt->anInterruptMask = FM10000_AN73_INT_MASK;
        }
        else if ( anMode == FM_PORT_AUTONEG_CLAUSE_37 ||
                  anMode == FM_PORT_AUTONEG_SGMII )
        {
            portExt->anInterruptMask = FM10000_AN37_INT_MASK;
        }

        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                         port,
                         "anMode=%d anInterruptMask=0x%08x\n",
                         anMode,
                         portExt->anInterruptMask );

        /* now, whether or not we started a new state machine or we
           switched on the fly, tell the port to start autoneg */
        status = fm10000AnSendConfigEvent( sw, 
                                           port,
                                           FM10000_PORT_EVENT_AN_CONFIG_REQ,
                                           anMode,
                                           basepage,
                                           nextPages );

    }   /* if ( status == FM_OK && anReady == TRUE ) */

ABORT:
    return status;

}   /* end fm10000AnRestartOnNewConfig */


/*****************************************************************************/
/** fm10000AnValidateBasePage
 * \ingroup intPort
 *
 * \desc            Helper function to validate the auto-negotiation basepage
 *
 * \param[in]       sw is the ID of the switch to operate on
 * 
 * \param[in]       port is the ID of the port to operate on
 * 
 * \param[in]       mode is the current autoneg mode or that we're trying to
 *                  switch to
 * 
 * \param[in]       basepage is the autonegotiation base page
 * 
 * \param[out]      modBasePage is the pointer to a caller-allocated variable
 *                  where this function will return the modified basepage with
 *                  unsupported abilities cleared
 *
 * \return          FM_OK if successful
 *
 * \return          FM_ERR_UNSUPPORTED if the ability field in the basepage is
 *                  made of unsupported modes only
 * 
 *****************************************************************************/
fm_status fm10000AnValidateBasePage( fm_int     sw, 
                                     fm_int     port, 
                                     fm_uint32  mode,
                                     fm_uint64  basepage,
                                     fm_uint64 *modBasePage )
{
    fm_status status;
    fm_port  *portPtr;
    fm_uint   ability;
    fm_uint   unsupported;
    fm_uint32 capabilities;

    status       = FM_OK;
    portPtr      = GET_PORT_PTR( sw, port );
    capabilities = portPtr->capabilities;

    /* Note if AN basepage is configured before ethernet mode
     * the validation can't be done */
    if ( mode == FM_PORT_AUTONEG_CLAUSE_73 )
    {
        ability = FM_GET_FIELD64( basepage, FM10000_AN_73_BASE_PAGE_TX, A );

        if ( ability != 0 )
        {
            unsupported = ( ability & FM10000_AN73_UNSUPPORTED_ABILITIES );

            /* check if any unsupported abilities have been requested */
            if ( unsupported != 0 )
            {
                FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                                 port,
                                 "Unsupported Clause 73 abilities configured "
                                 "on port %d: 0x%08x\n",
                                 port,
                                 unsupported );
            }
            ability &= ( ~FM10000_AN73_UNSUPPORTED_ABILITIES );

            /* check if at least one supported ability has been requested */
            if ( ability == 0 )
            {
                FM_LOG_ERROR( FM_LOG_CAT_PORT_AUTONEG,
                              "No supported Clause 73 abilities configured "
                              "on port %d\n",
                              port);
                return FM_ERR_UNSUPPORTED;
            }

            if ((ability & FM10000_AN73_ABILITY_1000BASE_KX) &&
                !(capabilities & FM_PORT_CAPABILITY_SPEED_1G))
            {
                FM_LOG_ERROR( FM_LOG_CAT_PORT_AUTONEG,
                              "Request to advertise 1G-KX but port %d "
                              "does not support 1G speed.\n",
                              port);
                return FM_ERR_UNSUPPORTED;
            }

            if ((ability & FM10000_AN73_ABILITY_10GBASE_KR) &&
                !(capabilities & FM_PORT_CAPABILITY_SPEED_10G))
            {
                FM_LOG_ERROR( FM_LOG_CAT_PORT_AUTONEG,
                              "Request to advertise 10G-KR but port %d "
                              "does not support 10G speed.\n",
                              port);
                return FM_ERR_UNSUPPORTED;
            }

            if ((ability & FM10000_AN73_ABILITY_25GBASE_KR) &&
                !(capabilities & FM_PORT_CAPABILITY_SPEED_25G))
            {
                FM_LOG_ERROR( FM_LOG_CAT_PORT_AUTONEG,
                              "Request to advertise 25G-CR/KR but port %d "
                              "does not support 25G speed.\n",
                              port);
                return FM_ERR_UNSUPPORTED;
            }

            if ((ability & FM10000_AN73_ABILITY_40GBASE_KR4) &&
                !(capabilities & FM_PORT_CAPABILITY_SPEED_40G))
            {
                FM_LOG_ERROR( FM_LOG_CAT_PORT_AUTONEG,
                              "Request to advertise 40G-KR4 but port %d "
                              "does not support 40G speed.\n",
                              port);
                return FM_ERR_UNSUPPORTED;
            }

            if ((ability & FM10000_AN73_ABILITY_40GBASE_CR4) &&
                !(capabilities & FM_PORT_CAPABILITY_SPEED_40G))
            {
                FM_LOG_ERROR( FM_LOG_CAT_PORT_AUTONEG,
                              "Request to advertise 40G-CR4 but port %d "
                              "does not support 40G speed.\n",
                              port);
                return FM_ERR_UNSUPPORTED;
            }

            if ((ability & FM10000_AN73_ABILITY_100GBASE_KR4) &&
                !(capabilities & FM_PORT_CAPABILITY_SPEED_100G))
            {
                FM_LOG_ERROR( FM_LOG_CAT_PORT_AUTONEG,
                              "Request to advertise 100G-KR4 but port %d "
                              "does not support 100G speed.\n",
                              port);
                return FM_ERR_UNSUPPORTED;
            }

            if ((ability & FM10000_AN73_ABILITY_100GBASE_CR4) &&
                !(capabilities & FM_PORT_CAPABILITY_SPEED_100G))
            {
                FM_LOG_ERROR( FM_LOG_CAT_PORT_AUTONEG,
                              "Request to advertise 100G-CR4 but port %d "
                              "does not support 100G speed.\n",
                              port);
                return FM_ERR_UNSUPPORTED;
            }
        }

        FM_SET_FIELD64( *modBasePage, FM10000_AN_73_BASE_PAGE_TX, A, ability );
    }

    return status;

}   /* end fm10000AnValidateBasePage */


/*****************************************************************************/
/** fm10000AnEventHandler
 * \ingroup intPort
 *
 * \desc            Function that processes AN-level interrupts
 *
 * \param[in]       sw is the switch on which to operate 
 * 
 * \param[in]       epl is the ID of the EPL on which the event occurred
 *
 * \param[in]       lane is the ID of the lane on which the event occurred
 * 
 * \param[in]       anIp is the interrupt pending mask for this EPL lane
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000AnEventHandler( fm_int    sw, 
                                 fm_int    epl, 
                                 fm_int    lane,
                                 fm_uint32 anIp )
{
    fm_status       status;
    fm_int          serDes;
    fm_int          port;
    fm10000_lane   *laneExt;
    fm10000_port   *portExt;
    fm_switch      *switchPtr;

    status = fm10000MapEplLaneToSerdes( sw, epl, lane, &serDes );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_PORT_AUTONEG, status );

    /* only process it if a lane is associated to this serdes */
    laneExt = GET_LANE_EXT( sw, serDes );
    if ( laneExt != NULL )
    {
        /* only process it if the lane is currently mapped to an active port */
        portExt = laneExt->parentPortExt;
        if ( portExt != NULL )
        {
            port = portExt->base->portNumber;

            FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                             port,
                             "AN Interrupt on port %d (type %d): 0x%08x\n", 
                             port, 
                             portExt->anSmType,
                             anIp );

            /* prepare the event info structure */
            portExt->eventInfo.info.physLane = lane;

            if ( portExt->anSmType == FM10000_CLAUSE73_AN_STATE_MACHINE )
            {
                FM_LOG_DEBUG2_V2( FM_LOG_CAT_PORT_AUTONEG,
                    port,
                    "Sw#%d Port %d: AN73_IP: %s%s%s%s%s%s%s%s%s\n",
                    sw,
                    port,
                    FM_GET_BIT(anIp, FM10000_AN_IP, An73TransmitDisable) ? "TxD " : "",
                    FM_GET_BIT(anIp, FM10000_AN_IP, An73AbilityDetect) ? "AbiD " : "",
                    FM_GET_BIT(anIp, FM10000_AN_IP, An73AcknowledgeDetect) ? "AckD " : "",
                    FM_GET_BIT(anIp, FM10000_AN_IP, An73CompleteAcknowledge) ? "ComA " : "",
                    FM_GET_BIT(anIp, FM10000_AN_IP, An73NextPageWait) ? "NxtW " : "",
                    FM_GET_BIT(anIp, FM10000_AN_IP, An73MrPageRx) ? "PgRx " : "",
                    FM_GET_BIT(anIp, FM10000_AN_IP, An73AnGoodCheck) ? "GoodC " : "",
                    FM_GET_BIT(anIp, FM10000_AN_IP, An73AnGood) ? "Good " : "",
                    FM_GET_BIT(anIp, FM10000_AN_IP, An73ReceiveIdle) ? "IdlRx " : "");

                /* process the interrupts related to the Clause 73
                   state machine */
                status = NotifyClause73Events( sw, port, anIp );
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, port, status);

            }
            else if ( portExt->anSmType == FM10000_CLAUSE37_AN_STATE_MACHINE )
            {
                FM_LOG_DEBUG2_V2( FM_LOG_CAT_PORT_AUTONEG,
                     port,
                     "Sw#%d Port %d: IP: %s%s%s%s%s%s%s%s\n",
                     sw,
                     port,
                     FM_GET_BIT(anIp, FM10000_AN_IP, An37AnRestart) ? "Rst " : "",
                     FM_GET_BIT(anIp, FM10000_AN_IP, An37AbilityDetect) ? "AbiD " : "",
                     FM_GET_BIT(anIp, FM10000_AN_IP, An37AcknowledgeDetect) ? "AckD " : "",
                     FM_GET_BIT(anIp, FM10000_AN_IP, An37CompleteAcknowledge) ? "ComA " : "",
                     FM_GET_BIT(anIp, FM10000_AN_IP, An37NextPageWait) ? "NxtW " : "",
                     FM_GET_BIT(anIp, FM10000_AN_IP, An37IdleDetect) ? "IdleD " : "",
                     FM_GET_BIT(anIp, FM10000_AN_IP, An37LinkOk) ? "LnkO " : "",
                     FM_GET_BIT(anIp, FM10000_AN_IP, An37MrPageRx) ? "PgRx " : "");

                /* process the interrupts related to the Clause 73
                   state machine */
                status = NotifyClause37Events( sw, port, anIp );
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, port, status);
                
            }

        }   /* end if ( portExt != NULL ) */

    }   /* end if ( laneExt != NULL ) */


ABORT:
    switchPtr = GET_SWITCH_PTR( sw );
    status = switchPtr->MaskUINT32( sw, 
                                    FM10000_AN_IM(epl, lane),
                                    anIp,
                                    FALSE );
    return status;

}   /* end fm10000AnEventHandler */



/*****************************************************************************/
/** fm10000An73HcdToEthMode
 * \ingroup intPort
 *
 * \desc            Return ethMode for the highest common denominator ability
 *                  negotiated using Clause 73 auto-negotiation
 *
 * \param[in]       hcd is the Highest Common Denominator (AN73_HCD_XXX)
 *
 * \return          The corresponding ethernet mode (see ''fm_ethMode'')
 *
 *****************************************************************************/
fm_ethMode fm10000An73HcdToEthMode( fm_int hcd )
{
    fm_ethMode ethMode;

    switch (hcd)
    {
        case AN73_HCD_KX:
            ethMode = FM_ETH_MODE_1000BASE_KX;
            break;

        case AN73_HCD_10_KR:
            ethMode = FM_ETH_MODE_10GBASE_KR;
            break;

        case AN73_HCD_40_CR4:
            ethMode = FM_ETH_MODE_40GBASE_CR4;
            break;

        case AN73_HCD_40_KR4:
            ethMode = FM_ETH_MODE_40GBASE_KR4;
            break;

        case AN73_HCD_100_KR4:
            ethMode = FM_ETH_MODE_100GBASE_KR4;
            break;

        case AN73_HCD_100_CR4:
            ethMode = FM_ETH_MODE_100GBASE_CR4;
            break;

        case AN73_HCD_25_KR:
            ethMode = FM_ETH_MODE_25GBASE_KR;
            break;

        case AN73_HCD_25_CR:
            ethMode = FM_ETH_MODE_25GBASE_CR;
            break;

        default:
            /* This shouldn't happen */
            ethMode = FM_ETH_MODE_DISABLED;
            break;

    }   /* end switch (hcd) */

    return ethMode;

}   /* end fm10000An73HcdToEthMode */


/*****************************************************************************/
/** fm10000An73HCDStr
 * \ingroup intPort
 *
 * \desc            Return the name of the HCD string.
 *
 * \param[in]       value is the HCD numeric value.
 *
 * \return          Text name of the HCD.
 *
 *****************************************************************************/
fm_text fm10000An73HCDStr(fm_uint value)
{

    switch (value)
    {
        case AN73_HCD_INCOMPATIBLE_LINK:
            return "AN73_HCD_INCOMPATIBLE_LINK(0)";
        case AN73_HCD_10_KR:
            return "AN73_HCD_10_KR(1)";
        case AN73_HCD_KX4:
            return "AN73_HCD_KX4(2)";
        case AN73_HCD_KX:
            return "AN73_HCD_KX(3)";
        case AN73_HCD_40_KR4:
            return "AN73_HCD_40_KR4(4)";
        case AN73_HCD_40_CR4:
            return "AN73_HCD_40_CR4(5)";
        case AN73_HCD_100_CR10:
            return "AN73_HCD_100_CR10(6)";
        case AN73_HCD_100_KP4:
            return "AN73_HCD_100_KP4(7)";
        case AN73_HCD_100_KR4:
            return "AN73_HCD_100_KR4(8)";
        case AN73_HCD_100_CR4:
            return "AN73_HCD_100_CR4(9)";
        case AN73_HCD_25_KR:
            return "AN73_HCD_25_KR(10)";
        case AN73_HCD_25_CR:
            return "AN73_HCD_25_CR(11)";
        default: 
            return "AN73_HCD_INVALID";

    }   /* end switch (value) */

}  /* end fm10000An73HCDStr */




/*****************************************************************************/
/** fm10000AnAddNextPage
 * \ingroup intPort
 *
 * \desc            Add a new nextPage to the auto-negotiation NextPage list.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \param[in]       nextPage is the new page to add to the NextPage list.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_NO_FREE_RESOURCES if no more free nextPage.
 * 
 *****************************************************************************/
fm_status fm10000AnAddNextPage(fm_int sw, fm_int port, fm_uint64 nextPage)
{
    fm_status    status = FM_OK;
    fm_portAttr *portAttr;
    fm_int       curNumPages;

    VALIDATE_SWITCH(sw);

    portAttr = GET_PORT_ATTR( sw, port );

    if (!portAttr->autoNegNextPages.nextPages)
    {
        portAttr->autoNegNextPages.numPages = 0;
        portAttr->autoNegNextPages.nextPages = 
            fmAlloc(sizeof(fm_uint64) * MAX_NUM_NEXTPAGES);

        if (portAttr->autoNegNextPages.nextPages == NULL)
        {
            return FM_ERR_NO_MEM;
        }
    }

    curNumPages = portAttr->autoNegNextPages.numPages;

    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                     port,
                     "port = %d, curNumPages = %d, nextPage=0x%016llx\n",
                     port,
                     curNumPages,
                     nextPage);

    /* Make sure we have room for another page */
    if (curNumPages < MAX_NUM_NEXTPAGES)
    {
        /* Add new page to the list */
        portAttr->autoNegNextPages.nextPages[curNumPages] = nextPage;
        portAttr->autoNegNextPages.numPages++;

        /* Set the NP bit to 1 in the previous nextPage */
        if (curNumPages > 0)
        {
            portAttr->autoNegNextPages.nextPages[curNumPages-1] |=
            (FM_LITERAL_U64(1)  << FM10000_AN_73_NEXT_PAGE_TX_b_NP);
        }
    }
    else
    {
        status = FM_ERR_NO_FREE_RESOURCES;
    }

    return status;
}




/*****************************************************************************/
/** fm10000AnVerifyEeeNegotiation
 * \ingroup intPort
 *
 * \desc            Verify if EEE is supported by the port's partner.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \param[in]       ethMode is the Ethernet Mode negotiated.
 *
 * \return          FM_OK if successful.
 * 
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * 
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * 
 * \return          FM_ERR_NO_FREE_RESOURCES if no more free nextPage.
 * 
 *****************************************************************************/
fm_status fm10000AnVerifyEeeNegotiation(fm_int sw, fm_int port, fm_int ethMode)
{
    fm_status         status=FM_OK;
    fm_portAttr      *portAttr;
    fm10000_portAttr *portAttrExt;
    fm_int            curNumPages=0;
    fm_uint64         rxPage;

    portAttr    = GET_PORT_ATTR( sw, port );
    portAttrExt = GET_FM10000_PORT_ATTR(sw, port);

    portAttrExt->negotiatedEeeModeEnabled = FALSE;

    /* Go through all received next pages */
    while (curNumPages < portAttr->autoNegPartnerNextPages.numPages)
    {
        rxPage = portAttr->autoNegPartnerNextPages.nextPages[curNumPages++];

        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                         port,
                         "port = %d, autoNegMode = %d, ehtMode = %x  "
                         "curNumPages = %d, rxPage=0x%016llx\n",
                         port,
                         portAttr->autoNegMode,
                         ethMode,
                         curNumPages,
                         rxPage);

        if (( FM_GET_FIELD64(rxPage, FM10000_AN_73_NEXT_PAGE_RX, MU) ==
                             FM10000_AN_NEXTPAGE_EEE_MSG_CODE ) &&
            ( portAttr->autoNegMode == FM_PORT_AUTONEG_CLAUSE_73 ))
        {
            /* AN 73 */
            if ( (( ethMode == FM_ETH_MODE_10GBASE_KR ) && 
                  ( rxPage & FM10000_AN_73_NEXTPAGE_EEE_10GBASE_KR )) ||
                 (( ethMode == FM_ETH_MODE_1000BASE_KX ) &&
                  ( rxPage & FM10000_AN_73_NEXTPAGE_EEE_1000BASE_KX )) )
            {
                portAttrExt->negotiatedEeeModeEnabled = TRUE;
                break;
            }
        }
    }

    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                     port,
                     "port = %d, autoNegMode = %d -- "
                     "EEE %s SUPPORTED\n",
                     port,
                     portAttr->autoNegMode,
                     portAttrExt->negotiatedEeeModeEnabled ? "IS":"IS NOT" );

    return status;
}




/*****************************************************************************/
/** fm10000AnGetMaxSpeedAbilityAndMode
 * \ingroup intPort
 *
 * \desc            Helper function to retrieve the maximum port speed and 
 *                  laneMode based on the given basepage.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the ID of the port to operate on.
 *
 * \param[in]       mode is the autoneg mode at which max speed to be 
 *                  retrieved.
 *
 * \param[in]       basepage is the autonegotiation base page
 * 
 * \param[in]       nextPages is NOT DOCUMENTED.
 *
 * \param[out]      maxSpeed is where maximum configured speed as per basepage
 *                  is returned.
 *
 * \param[out]      laneMode is where single or multilane mode is returned.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000AnGetMaxSpeedAbilityAndMode(fm_int                sw, 
                                             fm_int                port, 
                                             fm_uint32             mode,
                                             fm_uint64             basepage,
                                             fm_anNextPages        nextPages,
                                             fm_uint32            *maxSpeed,
                                             fm_schedulerPortMode *laneMode)
{
    fm_status status;
    fm_uint   ability;
    fm_bool   is40GCapable;
    fm_bool   is100GCapable;
    fm_bool   is25GConfigInNextPage;

    status    = FM_OK;
    *maxSpeed = 0;
    *laneMode = FM_SCHED_PORT_MODE_NONE;

    switch (mode)
    {
        case FM_PORT_AUTONEG_CLAUSE_73:
            if (basepage == 0)
            {
                status = fm10000GetMultiLaneCapabilities(sw, 
                                                         port, 
                                                         &is40GCapable,
                                                         &is100GCapable);
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, port, status);

                ability = FM10000_AN73_SUPPORTED_ABILITIES; 
                if (!is40GCapable)
                {
                    ability &= ~FM10000_AN73_ABILITIES_40G; 
                }

                if (!is100GCapable)
                {
                    ability &= ~FM10000_AN73_ABILITIES_100G;
                }
            }
            else
            {
                ability = FM_GET_FIELD64( basepage, FM10000_AN_73_BASE_PAGE_TX, A );
            }

            status  = IsAn25GConfiguredInNextPage(sw,
                                                  port,
                                                  nextPages, 
                                                  &is25GConfigInNextPage);
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, port, status);

            if ( ability & FM10000_AN73_ABILITIES_100G)
            {
                *maxSpeed = 100000;
                *laneMode = FM_SCHED_PORT_MODE_QUAD;
            }
            else if (ability & FM10000_AN73_ABILITIES_40G)
            {
                *maxSpeed = 40000;
                *laneMode = FM_SCHED_PORT_MODE_QUAD;
            }
            else if ( ( ability & 
                        (FM10000_AN73_ABILITY_25GBASE_KR |
                         FM10000_AN73_ABILITY_25GBASE_CR) ) ||
                      ( is25GConfigInNextPage ) )
            {
                *maxSpeed = 25000;
                *laneMode = FM_SCHED_PORT_MODE_SINGLE;
            }
            else if (ability & FM10000_AN73_ABILITY_10GBASE_KR)
            {
                *maxSpeed = 10000;
                *laneMode = FM_SCHED_PORT_MODE_SINGLE;
            }
            else if (ability & FM10000_AN73_ABILITY_1000BASE_KX)
            {
                *maxSpeed = 2500;
                *laneMode = FM_SCHED_PORT_MODE_SINGLE;
            }
            /* else *maxSpeed = 0 as assigned above. */
            break;

        case FM_PORT_AUTONEG_CLAUSE_37:
            *maxSpeed = 1000;
            *laneMode = FM_SCHED_PORT_MODE_SINGLE;
            break;

        case FM_PORT_AUTONEG_SGMII:
            *maxSpeed = 1000;
            *laneMode = FM_SCHED_PORT_MODE_SINGLE;
            break;
        
        default:
            status = FM_ERR_UNSUPPORTED;
            break;
    }

ABORT:

    return status;   

}    /* end fm10000AnGetMaxSpeedAbilityAndMode */




/*****************************************************************************/
/** fm10000AnGetNextPageExtTechAbilityIndex
 * \ingroup intPort
 *
 * \desc            Get the index to the extended technology ability in the
 *                  next page array.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \param[in]       nextPages is the array of next pages.
 *
 * \param[in]       numPages is the size of valid next pages.
 *
 * \param[in]       extTechAbIndex is caller-allocated storage where the
 *                  function will place the index to the extended
 *                  technology ability in the next page array. If not found,
 *                  -1 is returned here.
 *
 * \param[in]       dbgStr is the debug string.
 *
 * \return          FM_OK if found.
 * 
 * \return          FM_ERR_NOT_FOUND if not found.
 * 
 *****************************************************************************/
fm_status fm10000AnGetNextPageExtTechAbilityIndex(fm_int        sw,
                                                  fm_int        port,
                                                  fm_uint64    *nextPages,
                                                  fm_int        numPages,
                                                  fm_uint      *extTechAbIndex,
                                                  fm_text       dbgStr)
{
    fm_status    err;
    fm_int       pageNum;
    fm_uint64    pageA;
    fm_uint64    pageB;
    fm_uint      val;
    fm_uint      oui;
    fm_uint      cnt;
    fm_portAttr *portAttr;

    if ( extTechAbIndex == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    portAttr = GET_PORT_ATTR(sw, port);
    *extTechAbIndex = -1;
    err = FM_ERR_NOT_FOUND;

    /* Go through all received next pages if HCD from basepage
       resolved to less than 25G to find extended tech ability there */
    for (pageNum = 0; pageNum < numPages; pageNum++)
    {
        pageA = nextPages[pageNum];

        if (( FM_GET_FIELD64(pageA, FM10000_AN_73_NEXT_PAGE_RX, MU) == 
                             FM10000_AN_NEXTPAGE_OUI_MSG_CODE ) )
        {
            if (pageNum >= numPages)
            {
                FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                                 port,
                                 "Sw#%d port = %d, No unformatted next page\n",
                                 sw,
                                 port);
            }
            else
            {
                /* Got unformatted next page */
                pageB = nextPages[pageNum+1];
                val = FM_GET_UNNAMED_FIELD64(pageB, 0, 9);
                FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                                 port,
                                 "Sw#%d port = %d, OUI %sNextPage#%d=0x%016llx "
                                 "NextPage#%d=0x%016llx, extTechAbi=0x%x\n",
                                 sw,
                                 port,
                                 dbgStr,
                                 pageNum,
                                 pageA,
                                 pageNum+1,
                                 pageB,
                                 val);
                 /* Only if message is "Extended technology ability" */
                if (val == 0x3)
                {
                    /* Bit ordering is a bit unexpected */
                    oui = 0;
                    for (cnt = 0; cnt < 2; cnt++)
                    {
                        oui |= (FM_GET_UNNAMED_BIT64(pageB, 9 + cnt) << cnt);
                    }
                    for (cnt = 0; cnt < 11; cnt++)
                    {
                        oui |= (FM_GET_UNNAMED_BIT64(pageA, 32 + cnt) << (cnt+2));
                    }
                    for (cnt = 0; cnt < 11; cnt++)
                    {
                        oui |= (FM_GET_UNNAMED_BIT64(pageA, 16 + cnt) << (cnt+13));
                    }

                    if ( oui == portAttr->autoNeg25GNxtPgOui )
                    {
                        FM_LOG_DEBUG2_V2( FM_LOG_CAT_PORT_AUTONEG,
                                          port,
                                          "Sw#%d port = %d, %s OUI=0x%8.8x "
                                          "KR1=%llu CR1=%llu KR2=%llu CR2=%llu "
                                          "AdvCL91=%llu AdvCL74=%llu ReqCL91=%llu ReqCL74=%llu\n",
                                          sw,
                                          port,
                                          dbgStr,
                                          oui,
                                          FM_GET_UNNAMED_BIT64(pageB, 19),
                                          FM_GET_UNNAMED_BIT64(pageB, 20),
                                          FM_GET_UNNAMED_BIT64(pageB, 24),
                                          FM_GET_UNNAMED_BIT64(pageB, 25),
                                          FM_GET_UNNAMED_BIT64(pageB, 40),
                                          FM_GET_UNNAMED_BIT64(pageB, 41),
                                          FM_GET_UNNAMED_BIT64(pageB, 42),
                                          FM_GET_UNNAMED_BIT64(pageB, 43));

                        *extTechAbIndex = pageNum + 1;
                        err = FM_OK;
                    }
                    else
                    {
                        FM_LOG_DEBUG2_V2( FM_LOG_CAT_PORT_AUTONEG,
                                          port,
                                          "Sw#%d port = %d, %s local OUI=0x%8.8x received OUI=0x%8.8x (not recognized)\n",
                                          sw,
                                          port,
                                          dbgStr,
                                          portAttr->autoNeg25GNxtPgOui,
                                          oui);
                        continue;
                    }


                }
            }
        }
    }

    return err;

} /* fm10000AnGetNextPageExtTechAbilityIndex */



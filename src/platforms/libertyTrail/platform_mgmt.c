/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_xcvr_mgmt.c
 * Creation Date:   June 2, 2014
 * Description:     Platform transceiver management functions.
 *
 * Copyright (c) 2014 - 2015, Intel Corporation
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

#define MAX_EEPROM_READ_RETRY  4
#define MAX_CONFIG_RETRY       4

/* Avoid calling fmAlloc for temporary variable */
#define MAX_TEMP_PORTS         96

/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

static fm_semaphore mgmtSem;
static fm_bool      pollingPendingTask = FALSE;
static fm_bool      enableMgmt = FALSE;


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/* IsSfppModule1000BaseT
 * \ingroup intPlatform
 *
 * \desc            Return whether the SFP+ module is 1000BaseT.
 *
 * \param[in]       xcvrInfo is the pointer to transceiver info structure.
 *
 * \return          TRUE if module is 1000BaseT.
 *
 *****************************************************************************/
static fm_bool IsSfppModule1000BaseT(fm_platXcvrInfo *xcvrInfo)
{
    if (xcvrInfo->eepromBaseValid)
    {
        return fmPlatformXcvrIs1000BaseT(xcvrInfo->eeprom);
    }

    return FALSE;

}   /* end IsSfppModule1000BaseT */



/*****************************************************************************/
/* IsSfppModuleDualRate
 * \ingroup intPlatform
 *
 * \desc            Return whether the SFP+ module support dual-rate.
 *
 * \param[in]       xcvrInfo is the pointer to transceiver info structure.
 *
 * \return          TRUE if module support 1G/10G speed.
 *
 *****************************************************************************/
static fm_bool IsSfppModuleDualRate(fm_platXcvrInfo *xcvrInfo)
{
    if (xcvrInfo->eepromBaseValid)
    {
        return fmPlatformXcvrIs10G1G(xcvrInfo->eeprom);
    }

    return FALSE;

}   /* end IsSfppModuleDualRate */




/*****************************************************************************/
/* IsSfppPort1G
 * \ingroup intPlatform
 *
 * \desc            Return whether the SFP+ port is configured in 1G mode.
 *
 * \param[in]       xcvrInfo is the pointer to transceiver info structure.
 *
 * \return          TRUE if port is configured in 1G mode.
 *
 *****************************************************************************/
static fm_bool IsSfppPort1G(fm_platXcvrInfo *xcvrInfo)
{
    switch (xcvrInfo->ethMode)
    {
        case FM_ETH_MODE_DISABLED:
        case FM_ETH_MODE_SGMII:
        case FM_ETH_MODE_1000BASE_X:
        case FM_ETH_MODE_1000BASE_KX:
            return TRUE;

        default:
            return FALSE;
    }
}   /* end IsSfppPort1G */



/*****************************************************************************/
/* IsPortAnEnabled
 * \ingroup intPlatform
 *
 * \desc            Return where AN is enabled on the given port.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the port number.
 *
 * \return          TRUE if port has AN enabled.
 *
 *****************************************************************************/
static fm_bool IsPortAnEnabled(fm_int sw, fm_int port)
{
    fm_int    anEnabled;

    if (FM_OK == fmGetPortAttributeV2(sw,
                                      port,
                                      0,
                                      0,
                                      FM_PORT_AUTONEG,
                                      &anEnabled))
    {

        return (anEnabled == FM_PORT_AUTONEG_SGMII ||
                anEnabled == FM_PORT_AUTONEG_CLAUSE_37);
    }

    return FALSE;

}   /* end IsPortAnEnabled */



/*****************************************************************************/
/* ConfigureSfppXcvr
 * \ingroup intPlatform
 *
 * \desc            Perform any configuration to the SFP+ transceiver when
 *                  the module is brought up.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[in]       xcvrInfo is the pointer to transceiver info structure for
 *                  the specified port.
 *
 * \return          None.
 *
 *****************************************************************************/

static fm_status ConfigureSfppXcvr(fm_int sw,
                                   fm_int port,
                                   fm_platXcvrInfo *xcvrInfo)
{
    fm_status status;
    fm_byte   data;
    fm_bool   enable;

    MOD_STATE_DEBUG("Port %d:%d Config Xcvr DualRate %d 1000BaseT %d AN %d\n",
                    sw, port,
                    IsSfppModuleDualRate(xcvrInfo),
                    IsSfppModule1000BaseT(xcvrInfo),
                    IsPortAnEnabled(sw, port));

    if (!(xcvrInfo->modState & FM_PLAT_XCVR_ENABLE))
    {
        MOD_STATE_DEBUG("Port %d:%d Xcvr is not enabled to config\n", sw, port);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }

    if ( IsSfppModuleDualRate(xcvrInfo) )
    {
        /* Per SFF-8472, Table 3.17 */
        data = IsSfppPort1G(xcvrInfo) ? 0x0 : 0x8;
        status = fmPlatformXcvrMemWrite(sw, port, 1, 110, &data, 1);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* NOTE: Some SFP+ have separate rate
         * control for RX and TX. It doesn't hurt to set the same for other 
         * SFP+, that don't have this support */
        status = fmPlatformXcvrMemWrite(sw, port, 1, 118, &data, 1);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        MOD_STATE_DEBUG("Port %d:%d Force dual-rate module to %s\n",
                        sw, port, 
                        IsSfppPort1G(xcvrInfo) ? "1G" : "10G");
    }

    if ( IsSfppModule1000BaseT(xcvrInfo) )
    {
        enable = IsPortAnEnabled(sw, port);
        if (xcvrInfo->anEnabled != enable)
        {
            status = fmPlatformPhyEnable1000BaseTAutoNeg(sw, port, enable);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            MOD_STATE_DEBUG("Port %d:%d %s 1000BaseT autoneg\n",
                            sw, port, enable ? "enable" : "disable");
        }
        else
        {
            MOD_STATE_DEBUG("Port %d:%d 1000BaseT autoneg is already %s\n",
                            sw, port, enable ? "enable" : "disable");
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end ConfigureSfppXcvr */




/*****************************************************************************/
/* UpdateSerdesSettings
 * \ingroup intPlatform
 *
 * \desc            Update SERDES settings for the given port index.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       portIndex is the index to the port and transceiver
 *                  info structure.
 *
 * \return          None.
 *
 *****************************************************************************/
static void UpdateSerdesSettings(fm_int sw, fm_int portIndex)
{
    fm_int              lane;
    fm_int              epl;
    fm_platXcvrInfo *   xcvrInfo;
    fm_platformCfgPort *portCfg;

    portCfg  = FM_PLAT_GET_PORT_CFG(sw, portIndex);
    xcvrInfo = &GET_PLAT_STATE(sw)->xcvrInfo[portIndex];

    if (portCfg->intfType == FM_PLAT_INTF_TYPE_SFPP)
    {
        fmPlatformSetPortSerdesTxCfg(sw,
                                     portCfg->port,
                                     FALSE,
                                     xcvrInfo->ethMode);

    }
    else if (portCfg->intfType == FM_PLAT_INTF_TYPE_QSFP_LANE0)
    {
        if (xcvrInfo->ethMode & FM_ETH_MODE_MULTI_LANE_MASK)
        {
            fmPlatformSetPortSerdesTxCfg(sw,
                                         portCfg->port,
                                         TRUE,
                                         xcvrInfo->ethMode);
        }
        else
        {
            /* 4 individual ports */
            epl = portCfg->epl;
            for (lane = 0 ; lane < 4 ; lane++)
            {
                /* Get the portIndex associated to the lane.*/
                portIndex = 
                    FM_PLAT_GET_SWITCH_CFG(sw)->epls[epl].laneToPortIdx[lane];

                if (portIndex != FM_PLAT_UNDEFINED)
                {
                    portCfg = FM_PLAT_GET_PORT_CFG(sw, portIndex);
                    xcvrInfo = &GET_PLAT_STATE(sw)->xcvrInfo[portIndex];
                    fmPlatformSetPortSerdesTxCfg(sw,
                                                 portCfg->port,
                                                 FALSE,
                                                 xcvrInfo->ethMode);
                }
            }
        }
    }

}   /* end UpdateSerdesSettings */




/*****************************************************************************/
/* UpdateXcvrConfig
 * \ingroup intPlatform
 *
 * \desc            Update transceiver configuration for the given port index.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       portIndex is the index to the port and transceiver
 *                  info structure.
 *
 * \return          None.
 *
 *****************************************************************************/
static void UpdateXcvrConfig(fm_int sw, fm_int portIndex, fm_int retries)
{
    fm_platXcvrInfo *   xcvrInfo;
    fm_platformCfgPort *portCfg;

    portCfg  = FM_PLAT_GET_PORT_CFG(sw, portIndex);
    xcvrInfo = &GET_PLAT_STATE(sw)->xcvrInfo[portIndex];

    if (portCfg->intfType == FM_PLAT_INTF_TYPE_SFPP)
    {
        xcvrInfo->configRetries = retries - 1;
        if (ConfigureSfppXcvr(sw, portCfg->port, xcvrInfo))
        {
            xcvrInfo->configRetries = retries - 1;
        }
        else
        {
            xcvrInfo->configRetries = 0;
        }
    }
    else if (portCfg->intfType == FM_PLAT_INTF_TYPE_QSFP_LANE0)
    {
        /* Nothing to do now */
    }

}   /* end UpdateXcvrConfig */




/*****************************************************************************/
/* XcvrReadAndValidateEeprom
 * \ingroup intPlatform
 *
 * \desc            Read and validate transceiver eeprom content.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       portIndex is the index to the transceiver info structure.
 *
 * \param[in]       retry indicates whether it is a retry read or not.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status XcvrReadAndValidateEeprom(fm_int  sw,
                                           fm_int  portIndex,
                                           fm_bool retry)
{
    fm_platformCfgPort *portCfg;
    fm_platXcvrInfo *   xcvrInfo;
    fm_status           status;

    portCfg = FM_PLAT_GET_PORT_CFG(sw, portIndex);
    xcvrInfo = &GET_PLAT_STATE(sw)->xcvrInfo[portIndex];

    status = fmPlatformXcvrEepromRead(sw,
                                      portCfg->port,
                                      0,
                                      0,
                                      xcvrInfo->eeprom,
                                      XCVR_EEPROM_CACHE_SIZE);
    if (status == FM_OK)
    {
        xcvrInfo->eepromReadRetries = 0;
        xcvrInfo->eepromBaseValid =
            fmPlatformXcvrEepromIsBaseCsumValid(xcvrInfo->eeprom);
        xcvrInfo->eepromExtValid =
            fmPlatformXcvrEepromIsExtCsumValid(xcvrInfo->eeprom);

        if (xcvrInfo->eepromBaseValid)
        {
            xcvrInfo->type = fmPlatformXcvrEepromGetType(xcvrInfo->eeprom);
            xcvrInfo->cableLength = 
                fmPlatformXcvrEepromGetLen(xcvrInfo->eeprom);
        }
        else
        {
            xcvrInfo->type = FM_PLATFORM_XCVR_TYPE_UNKNOWN;
            xcvrInfo->cableLength = 0;
        }

        MOD_TYPE_DEBUG("Port %d:%d Transceiver type: %s length: %d\n",
                       sw,
                       portCfg->port, 
                       fmPlatformXcvrTypeGetName(xcvrInfo->type), 
                       xcvrInfo->cableLength);
    }
    else if (!retry)
    {
        /* Some modules need a while to response.
         * So we will just mark it here for the polling
         * thread to handle it later */

        xcvrInfo->eepromReadRetries = MAX_EEPROM_READ_RETRY;
        xcvrInfo->type = FM_PLATFORM_XCVR_TYPE_UNKNOWN;

        MOD_STATE_DEBUG("Port %d:%d Failed to read module EEPROM\n", 
                        sw, portCfg->port);
    }

    return status;

}   /* end XcvrReadAndValidateEeprom */




/*****************************************************************************/
/* XcvrUpdateState
 * \ingroup intPlatform
 *
 * \desc            Update transceiver state, normally called when there is an
 *                  interrupt notifying state change, or polling.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       force is update state even without state change.
 *
 * \param[in]       interrupting indicates interrupts are pending.
 *
 * \return          None.
 *
 *****************************************************************************/
static void XcvrUpdateState(fm_int sw, fm_bool force, fm_bool interrupting)
{
    fm_status           status;
    fm_int              portIdx;
    fm_int              lanePortIdx;
    fm_int              hwResIdIdx;
    fm_int              port;
    fm_int              lane;
    fm_int              epl;
    fm_int              swNum;
    fm_platformLib     *libFunc;
    fm_platformCfgPort *portCfg;
    fm_platformCfgPort *pCfg;
    fm_platXcvrInfo *   xcvrInfo;
    fm_uint32           xcvrSignals;
    fm_uint32           xcvrState;
    fm_uint32           xcvrStateValid;
    fm_uint32           oldState;
    fm_bool             present;
    fm_bool             notify;
    fm_int              numPorts;
    fm_uint32           hwResIdList[MAX_TEMP_PORTS];
    fm_int              hwResIdIdxList[MAX_TEMP_PORTS];
    fm_uint32           xcvrStateValidList[MAX_TEMP_PORTS];
    fm_uint32           xcvrStateList[MAX_TEMP_PORTS];
    fm_int              numPortsIntr;
    fm_int              cnt;

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    if ( !libFunc->GetPortXcvrState )
    {
        /* No support */
        return;
    }

    swNum = FM_PLAT_GET_SWITCH_CFG(sw)->swNum;
    xcvrInfo = GET_PLAT_STATE(sw)->xcvrInfo;

    numPortsIntr = 0;
    numPorts     = 0;

    if ( interrupting && libFunc->GetPortIntrPending )
    {
        TAKE_PLAT_I2C_BUS_LOCK(sw);
        status = libFunc->GetPortIntrPending(swNum,
                                             hwResIdList,
                                             MAX_TEMP_PORTS,
                                             &numPortsIntr);
        DROP_PLAT_I2C_BUS_LOCK(sw);

        if (status == FM_OK)
        {
            if (fmRootPlatform->cfg.debug & CFG_DBG_MOD_INTR)
            {
                FM_LOG_PRINT("Switch %d: Interrupt pending ports:", sw);
            }

            for (cnt = 0 ; cnt < numPortsIntr ; cnt++)
            {
                /* Better search logic ?? */
                for (portIdx = 0 ; portIdx < FM_PLAT_NUM_PORT(sw) ; portIdx++)
                {
                    portCfg = FM_PLAT_GET_PORT_CFG(sw, portIdx);

                    if (portCfg->hwResourceId == hwResIdList[cnt])
                    {
                        if (fmRootPlatform->cfg.debug & CFG_DBG_MOD_INTR)
                        {
                            FM_LOG_PRINT(" %d", portCfg->port);
                        }

                        hwResIdIdxList[cnt] = portIdx;
                        numPorts++;
                    }
                }
            }

            if (fmRootPlatform->cfg.debug & CFG_DBG_MOD_INTR)
            {
                FM_LOG_PRINT("\n");
            }

            if (numPorts != numPortsIntr)
            {
                /* Some issue converting hwResId to our logical ports */
                FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                              "Unexpected mismatch numPorts %d numPortsIntr %d\n",
                              numPorts, numPortsIntr);
                FM_LOG_PRINT("HwResourceIdList: ");
                for (cnt = 0 ; cnt < numPortsIntr ; cnt++)
                {
                    FM_LOG_PRINT(" %d", hwResIdList[cnt]);
                }
                FM_LOG_PRINT("\n");
                FM_LOG_PRINT("hwResIdIdxList: ");
                for (cnt = 0 ; cnt < numPorts ; cnt++)
                {
                    FM_LOG_PRINT(" %d", hwResIdIdxList[cnt]);
                }
                FM_LOG_PRINT("\n");
            }
        }
    }

    if (numPortsIntr <= 0)
    {
        /* Update for all ports */
        numPorts = 0;

        for (portIdx = 0 ; portIdx < FM_PLAT_NUM_PORT(sw) ; portIdx++)
        {
            portCfg = FM_PLAT_GET_PORT_CFG(sw, portIdx);

            if ( !(portCfg->intfType == FM_PLAT_INTF_TYPE_SFPP ||
                   portCfg->intfType == FM_PLAT_INTF_TYPE_QSFP_LANE0) )
            {
                continue;
            }

            hwResIdList[numPorts]    = portCfg->hwResourceId;
            hwResIdIdxList[numPorts] = portIdx;
            numPorts++;
        }
    }

    if (numPorts == 0)
    {
        MOD_STATE_DEBUG("Switch %d: No port to process\n", sw);
        return;
    }

    /* Get transceiver state */
    TAKE_PLAT_I2C_BUS_LOCK(sw);

    if ( libFunc->SelectBus )
    {
        status = libFunc->SelectBus(swNum, FM_PLAT_BUS_XCVR_STATE, hwResIdList[0]);

        if (status)
        {
            MOD_STATE_DEBUG("Switch %d: Failed to select transceiver bus. %s\n",
                            sw, 
                            fmErrorMsg(status) );
        }

        /* Continue in order to drop I2C lock */
    }

    status = libFunc->GetPortXcvrState(swNum,
                                       hwResIdList,
                                       numPorts,
                                       xcvrStateValidList,
                                       xcvrStateList);

    DROP_PLAT_I2C_BUS_LOCK(sw);

    if (status)
    {
        MOD_STATE_DEBUG("Switch %d: Failed to read transceiver state. %s\n",
                        sw, 
                        fmErrorMsg(status) );
        return;
    }


    for (hwResIdIdx = 0 ; hwResIdIdx < numPorts ; hwResIdIdx++)
    {
        portIdx = hwResIdIdxList[hwResIdIdx];

        portCfg = FM_PLAT_GET_PORT_CFG(sw, portIdx);

        if ( !(portCfg->intfType == FM_PLAT_INTF_TYPE_SFPP ||
               portCfg->intfType == FM_PLAT_INTF_TYPE_QSFP_LANE0) )
        {
            FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                         "Unexpected port interface type: %s\n",
                         fmErrorMsg(status) );
            continue;
        }

        port = portCfg->port;

        xcvrStateValid = xcvrStateValidList[hwResIdIdx];
        xcvrState      = xcvrStateList[hwResIdIdx];
        oldState       = xcvrInfo[portIdx].modState;
        present        = (xcvrState & FM_PLAT_XCVR_PRESENT);
        notify         = FALSE;

        if (oldState ^ xcvrState)
        {
            /* Save the new state */
            xcvrInfo[portIdx].modState = xcvrState;

            if ( (xcvrStateValid & FM_PLAT_XCVR_PRESENT) &&
                 ( (oldState & FM_PLAT_XCVR_PRESENT) != 
                  (xcvrState & FM_PLAT_XCVR_PRESENT) ) )
            {
                notify = TRUE;
                MOD_STATE_DEBUG("Port %d module PRESENCE status is "
                                "changed to %s\n",
                                port, 
                                present ? "PRESENT" : "NOT_PRESENT");

                xcvrInfo[portIdx].type = FM_PLATFORM_XCVR_TYPE_NOT_PRESENT;
                xcvrInfo[portIdx].cableLength       = 0;
                xcvrInfo[portIdx].present           = present;
                xcvrInfo[portIdx].eepromBaseValid   = FALSE;
                xcvrInfo[portIdx].eepromExtValid    = FALSE;
                xcvrInfo[portIdx].eepromReadRetries = 0;
                FM_MEMSET_S(xcvrInfo->eeprom, 
                            sizeof(xcvrInfo->eeprom),
                            0xFF, 
                            sizeof(xcvrInfo->eeprom));
            }

            if ( (xcvrStateValid & FM_PLAT_XCVR_ENABLE) &&
                (oldState & FM_PLAT_XCVR_ENABLE) != (xcvrState & FM_PLAT_XCVR_ENABLE) )
            {
                notify = TRUE;
                MOD_STATE_DEBUG("Port %d module ENABLE status is changed to %d\n",
                                port, 
                                (xcvrState & FM_PLAT_XCVR_ENABLE) ? TRUE : FALSE);
                xcvrInfo->configRetries     = 0;
                xcvrInfo->anEnabled         = 0;
            }

            if (notify && present && (xcvrState & FM_PLAT_XCVR_ENABLE))
            {
                status = XcvrReadAndValidateEeprom(sw, portIdx, FALSE);
                MOD_STATE_DEBUG("Port %d module reading EEPROM: %s\n",
                                port, 
                                fmErrorMsg(status));
                if (status == FM_OK)
                {
                    UpdateSerdesSettings(sw, portIdx);
                    UpdateXcvrConfig(sw, portIdx, MAX_CONFIG_RETRY);
                }
            }

            if ( (xcvrStateValid & FM_PLAT_XCVR_RXLOS) &&
                (oldState & FM_PLAT_XCVR_RXLOS) != (xcvrState & FM_PLAT_XCVR_RXLOS) )
            {
                notify = TRUE;
                MOD_STATE_DEBUG("Port %d module RXLOS status is changed to %d\n",
                                port, 
                                (xcvrState & FM_PLAT_XCVR_RXLOS) ? TRUE : FALSE);
            }

            if ( (xcvrStateValid & FM_PLAT_XCVR_TXFAULT) &&
                (oldState & FM_PLAT_XCVR_TXFAULT) != (xcvrState & FM_PLAT_XCVR_TXFAULT) )
            {
                notify = TRUE;
                MOD_STATE_DEBUG("Port %d module TXFAULT status is changed to %d\n",
                                port, 
                                (xcvrState & FM_PLAT_XCVR_TXFAULT) ? TRUE : FALSE);
            }

            if ( (xcvrStateValid & FM_PLAT_XCVR_INTR) &&
                (oldState & FM_PLAT_XCVR_INTR) != (xcvrState & FM_PLAT_XCVR_INTR) )
            {
                MOD_STATE_DEBUG("Port %d module INTR status is changed to %d\n",
                                port, 
                                (xcvrState & FM_PLAT_XCVR_INTR) ? TRUE : FALSE);
            }
        }

        if (notify || force)
        {
            xcvrSignals = 0;

            if (present)
            {
                xcvrSignals |= FM_PORT_XCVRSIG_MODPRES;
            }

            if (portCfg->intfType == FM_PLAT_INTF_TYPE_SFPP)
            {
                if (xcvrState & FM_PLAT_XCVR_RXLOS)
                {
                    xcvrSignals |= FM_PORT_XCVRSIG_RXLOS;
                }

                if (xcvrState & FM_PLAT_XCVR_TXFAULT)
                {
                    xcvrSignals |= FM_PORT_XCVRSIG_TXFAULT;
                }

                /* Notify the API only if the ethernet mode not DISABLED */
                if (xcvrInfo[portIdx].ethMode != FM_ETH_MODE_DISABLED)
                {
                    /* Notify the API */
                    fmNotifyXcvrChange(sw, port, 0, 0, xcvrSignals, NULL);
                    MOD_STATE_DEBUG("Port %d Notify API xcvrSignals 0x%x\n",
                                    port, 
                                    xcvrSignals);
                }

                /* Notify the Application */
                fmPlatformEventSendPortXcvrState(sw,
                                                 port,
                                                 0,
                                                 FM_PORT_LANE_NA,
                                                 xcvrSignals,
                                                 FM_EVENT_PRIORITY_LOW);

                MOD_STATE_DEBUG("Port %d Notify APP xcvrSignals 0x%x\n",
                                port, 
                                xcvrSignals);
            }
            else if (portCfg->intfType == FM_PLAT_INTF_TYPE_QSFP_LANE0)
            {
                if ( !(xcvrInfo[portIdx].ethMode & 
                       FM_ETH_MODE_MULTI_LANE_MASK) )
                {
                    /* 4 individual ports */
                    epl = portCfg->epl;
                    for (lane = 0 ; lane < 4 ; lane++)
                    {
                        /* Get the port index associated to the lane.*/
                        lanePortIdx = 
                            FM_PLAT_GET_SWITCH_CFG(sw)->epls[epl].laneToPortIdx[lane];

                        if (lanePortIdx == FM_PLAT_UNDEFINED)
                        {
                            /* No logical port associated to this epl/lane */
                            continue;
                        }

                        /* Notify the API only if the eth_mode is not DISABLED */
                        if (xcvrInfo[lanePortIdx].ethMode != FM_ETH_MODE_DISABLED)
                        {
                            pCfg = FM_PLAT_GET_PORT_CFG(sw, lanePortIdx);

                            /* Notify the API */
                            fmNotifyXcvrChange(sw, 
                                               pCfg->port, 
                                               0, 
                                               0, 
                                               xcvrSignals, 
                                               NULL);

                            MOD_STATE_DEBUG("Port %d Notify API "
                                            "xcvrSignals 0x%x\n",
                                            pCfg->port, 
                                            xcvrSignals);
                        }
                    }

                    /* Notify the Application */
                    fmPlatformEventSendPortXcvrState(sw,
                                                     port,
                                                     0,
                                                     FM_PORT_LANE_NA,
                                                     xcvrSignals,
                                                     FM_EVENT_PRIORITY_LOW);

                    MOD_STATE_DEBUG("Port %d Notify APP xcvrSignals 0x%x\n",
                                    port, 
                                    xcvrSignals);
                }
                else
                {
                    /* Should use LANE_ALL, but not supported in API */
                    for (lane = 0 ; lane < 4 ; lane++)
                    {
                        /* Notify the API only if the eth_mode not DISABLED */
                        if (xcvrInfo[portIdx].ethMode != FM_ETH_MODE_DISABLED)
                        {
                            /* Notify the API */
                            fmNotifyXcvrChange(sw, 
                                               port, 
                                               0, 
                                               lane, 
                                               xcvrSignals, 
                                               NULL);

                            MOD_STATE_DEBUG("Port %d.%d Notify API "
                                            "xcvrSignals 0x%x\n",
                                            port, 
                                            lane, 
                                            xcvrSignals);
                        }
                    }

                    /* Notify the Application */
                    fmPlatformEventSendPortXcvrState(sw,
                                                     port,
                                                     0,
                                                     FM_PORT_LANE_ALL,
                                                     xcvrSignals,
                                                     FM_EVENT_PRIORITY_LOW);

                    MOD_STATE_DEBUG("Port %d Notify APP xcvrSignals 0x%x\n",
                                    port, 
                                    xcvrSignals);
                }
            }

        }   /* end if (notify || force) */

    }   /* end for (hwResIdIdx = 0 ; hwResIdIdx < numPorts ; hwResIdIdx++) */

}   /* end XcvrUpdateState */




/*****************************************************************************/
/* XcvrRetryEepromRead
 * \ingroup intPlatformMgmt
 *
 * \desc            Check for SFP+ or QSFP module that need to retry reading
 *                  EEPROM and update SERDES settings when able to read
 *                  the module EEPROM content successfully.
 *
 * \param[in]       sw is the switch number.
 * 
 * \return          None.
 *
 *****************************************************************************/
static void XcvrRetryEepromRead(fm_int sw)
{
    fm_status           status;
    fm_int              portIdx;
    fm_platformCfgPort *portCfg;
    fm_platformLib     *libFunc;
    fm_platXcvrInfo *   xcvrInfo;

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    if ( !libFunc->I2cWriteRead )
    {
        /* No support */
        return;
    }

    for (portIdx = 0 ; portIdx < FM_PLAT_NUM_PORT(sw) ; portIdx++)
    {
        portCfg = FM_PLAT_GET_PORT_CFG(sw, portIdx);
        xcvrInfo = &GET_PLAT_STATE(sw)->xcvrInfo[portIdx];

        if ( portCfg->intfType != FM_PLAT_INTF_TYPE_SFPP &&
             portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE0 )
        {
            continue;
        }

        if (xcvrInfo->eepromReadRetries > 0)
        {
            xcvrInfo->eepromReadRetries--;

            status = XcvrReadAndValidateEeprom(sw, portIdx, TRUE);
            if (status != FM_OK)
            {
                if (xcvrInfo->eepromReadRetries == 0)
                {
                    MOD_STATE_DEBUG("Port %d:%d Reading module EEPROM failed\n",
                                     sw,
                                     portCfg->port);
                }
                continue;
            }

            MOD_STATE_DEBUG("Port %d:%d Reading module EEPROM "
                            "success in %d tries\n",
                             sw,
                             portCfg->port,
                             MAX_EEPROM_READ_RETRY - xcvrInfo->eepromReadRetries);

            xcvrInfo->eepromReadRetries = 0;
            UpdateSerdesSettings(sw, portIdx);
        }
    }

}   /* end XcvrRetryEepromRead */



/*****************************************************************************/
/* XcvrRetryConfig
 * \ingroup intPlatformMgmt
 *
 * \desc            Check for SFP+ or QSFP module that need to retry configuring.
 *
 * \param[in]       sw is the switch number.
 * 
 * \return          None.
 *
 *****************************************************************************/
static void XcvrRetryConfig(fm_int sw)
{
    fm_int              portIdx;
    fm_platformCfgPort *portCfg;
    fm_platformLib     *libFunc;
    fm_platXcvrInfo *   xcvrInfo;

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    if ( !libFunc->I2cWriteRead )
    {
        /* No support */
        return;
    }

    for (portIdx = 0 ; portIdx < FM_PLAT_NUM_PORT(sw) ; portIdx++)
    {
        portCfg = FM_PLAT_GET_PORT_CFG(sw, portIdx);
        xcvrInfo = &GET_PLAT_STATE(sw)->xcvrInfo[portIdx];

        if ( portCfg->intfType != FM_PLAT_INTF_TYPE_SFPP &&
             portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE0 )
        {
            continue;
        }

        if (xcvrInfo->eepromBaseValid && xcvrInfo->configRetries > 0)
        {
            xcvrInfo->configRetries--;

            if ((portCfg->intfType == FM_PLAT_INTF_TYPE_SFPP))
            {
                if (FM_OK == ConfigureSfppXcvr(sw, portCfg->port, xcvrInfo))
                {
                    MOD_STATE_DEBUG("Port %d:%d Config SFP+ module "
                                    "success in %d tries\n",
                                     sw,
                                     portCfg->port,
                                     MAX_CONFIG_RETRY - xcvrInfo->configRetries);

                    xcvrInfo->configRetries = 0;
                }
                else if (xcvrInfo->configRetries == 0)
                {
                    FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                                 "Failed to configure port %d SFP+ module\n", portCfg->port);
                }
            }
        }
    }

}   /* end XcvrRetryConfig */




/*****************************************************************************/
/* fmPlatformMgmtThread
 * \ingroup intPlatform
 *
 * \desc            thread to handle various MGMT and PHY info.
 *
 * \param[in]       args contains thread-initialization parameters
 *
 * \return          None.
 *
 *****************************************************************************/
static void *fmPlatformMgmtThread(void *args)
{
    fm_status    status;
    fm_thread *  thread;
    fm_int       sw;
    fm_bool      pollXcvrStatus;
    fm_timestamp timeout;
    fm_uint      xcvrPollPeriodMsec;
    fm_bool      interrupt;

    /* grab arguments */
    thread = FM_GET_THREAD_HANDLE(args);
    sw     = *(FM_GET_THREAD_PARAM(fm_int, args));

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "thread= %s, sw %d\n", thread->name, sw);

    status = fmCreateSemaphore("platformMgmtSem",
                               FM_SEM_BINARY,
                               &mgmtSem,
                               0);

    if (status != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                     "Unable to create platform mgmt semaphore: %s\n",
                     fmErrorMsg(status) );
    }

    if ( FM_PLAT_GET_SWITCH_CFG(sw)->gpioPortIntr == FM_PLAT_UNDEFINED )
    {
        /* No Interrupt support, just poll */
        pollXcvrStatus = TRUE;
    }
    else
    {
        pollXcvrStatus = FALSE;
    }

    xcvrPollPeriodMsec = FM_PLAT_GET_SWITCH_CFG(sw)->xcvrPollPeriodMsec;

    if (xcvrPollPeriodMsec)
    {
        timeout.sec  = xcvrPollPeriodMsec / 1000;
        timeout.usec = (xcvrPollPeriodMsec % 1000) * 1000;
    }
    else
    {
        timeout.sec  = 1;
        timeout.usec = 0;        
    }

    while (1)
    {
        /* Handle interrupt and polling */
        status = fmWaitSemaphore(&mgmtSem, &timeout);

        /* Status != OK means the semaphore timeout, so do polling */
        interrupt = (status == FM_OK) ? TRUE : FALSE;

        /* Don't start before switch is brought up */
        if (!enableMgmt)
        {
            continue;
        }

        if (fmPlatformMgmtTakeSwitchLock(sw) != FM_OK)
        {
            continue;
        }

        if (!interrupt || pollingPendingTask)
        {
            /* Do polling task here */
            pollingPendingTask = FALSE;

            /* Retry EEPROM reading when module is not ready */
            XcvrRetryEepromRead(sw);

            /* Retry configuring module */
            XcvrRetryConfig(sw);
        }

        if (interrupt || pollXcvrStatus)
        {
            /* Read SFP+ and QSFP state */
            XcvrUpdateState( sw, FALSE, interrupt );
        }

        fmPlatformMgmtDropSwitchLock(sw);

    }   /* end while (1) */

    return NULL;

}       /* end fmPlatformMgmtThread */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/



/*****************************************************************************/
/* fmPlatformMgmtTakeSwitchLock
 * \ingroup intPlatformMgmt
 *
 * \desc            Take switch lock for mgmt functions.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformMgmtTakeSwitchLock(fm_int sw)
{
    fm_status status = FM_OK;

    /* NOTE: For some configuration, we might not need to take switch lock
     *       But for now, we will take it regardless
     */
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(status, sw);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformMgmtTakeSwitchLock */



/*****************************************************************************/
/* fmPlatformMgmtDropSwitchLock
 * \ingroup intPlatformMgmt
 *
 * \desc            Drop switch lock for mgmt functions.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformMgmtDropSwitchLock(fm_int sw)
{
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformMgmtDropSwitchLock */



/*****************************************************************************/
/* fmPlatformMgmtInit
 * \ingroup intPlatformMgmt
 *
 * \desc            This function initializes peripheral functions.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformMgmtInit(fm_int sw)
{
    fm_status status = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    /* NOTE: This structure can be indexed by portIdx similar to
             fm_platformCfgPort */
    GET_PLAT_STATE(sw)->xcvrInfo = 
        fmAlloc( FM_PLAT_NUM_PORT(sw) * sizeof(fm_platXcvrInfo) );

    if (GET_PLAT_STATE(sw)->xcvrInfo == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_NO_MEM);
    }

    memset( GET_PLAT_STATE(sw)->xcvrInfo, 
            0, 
            FM_PLAT_NUM_PORT(sw) * sizeof(fm_platXcvrInfo) );

    if (FM_PLAT_GET_SWITCH_CFG(sw)->xcvrPollPeriodMsec <= 0)
    {
        FM_LOG_PRINT("Platform management thread disabled by config file.\n");
    }
    else
    {
        status = fmCreateThread("Mgmt Thread",
                                FM_EVENT_QUEUE_SIZE_NONE,
                                &fmPlatformMgmtThread,
                                &(GET_PLAT_STATE(sw)->sw),
                                &GET_PLAT_STATE(sw)->mgmtThread);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformMgmtInit */




/*****************************************************************************/
/* fmPlatformMgmtXcvrInitialize
 * \ingroup intPlatform
 *
 * \desc            Initialize the transceiver structure for all ports.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformMgmtXcvrInitialize(fm_int sw)
{
    fm_platformLib     *libFunc;
    fm_platformCfgPort *portCfg;
    fm_platXcvrInfo *   xcvrInfo;
    fm_int              hwResIdIdxList[MAX_TEMP_PORTS];
    fm_uint32           hwResIdList[MAX_TEMP_PORTS];
    fm_uint32           xcvrStateValidList[MAX_TEMP_PORTS];
    fm_uint32           xcvrStateList[MAX_TEMP_PORTS];
    fm_int              portIdx;
    fm_int              numPorts;
    fm_int              hwResIdIdx;
    fm_status           status;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    numPorts = 0;

    /**************************************************
     * Initialize the XCVR structure and create the 
     * hardware resource ID list.
     **************************************************/

    for (portIdx = 0 ; portIdx < FM_PLAT_NUM_PORT(sw) ; portIdx++)
    {
        portCfg = FM_PLAT_GET_PORT_CFG(sw, portIdx);
        xcvrInfo = &GET_PLAT_STATE(sw)->xcvrInfo[portIdx];

        xcvrInfo->ethMode           = portCfg->ethMode;
        xcvrInfo->modState          = 0;
        xcvrInfo->present           = FALSE;
        xcvrInfo->anEnabled         = FALSE;
        xcvrInfo->type              = FM_PLATFORM_XCVR_TYPE_NOT_PRESENT;
        xcvrInfo->cableLength       = 0;
        xcvrInfo->eepromBaseValid   = FALSE;
        xcvrInfo->eepromExtValid    = FALSE;
        xcvrInfo->eepromReadRetries = 0;
        xcvrInfo->configRetries     = 0;

        FM_MEMSET_S(xcvrInfo->eeprom, 
                    sizeof(xcvrInfo->eeprom),
                    0xFF, 
                    sizeof(xcvrInfo->eeprom));

        if ( !libFunc->GetPortXcvrState ||
             ( !(portCfg->intfType == FM_PLAT_INTF_TYPE_SFPP ||
                 portCfg->intfType == FM_PLAT_INTF_TYPE_QSFP_LANE0) ) )
        {
            continue;
        }

        /* Create the HW resource ID list */
        hwResIdList[numPorts]    = portCfg->hwResourceId;
        hwResIdIdxList[numPorts] = portIdx;
        numPorts++;
    }

    if (numPorts == 0 || !libFunc->GetPortXcvrState)
    {
        MOD_STATE_DEBUG("Switch %d: No port to process\n", sw);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }

    /**************************************************
     * Get the transceiver state to determine whether 
     * a module is present.
     **************************************************/

    TAKE_PLAT_I2C_BUS_LOCK(sw);

    if ( libFunc->SelectBus )
    {
        /* Select proper I2C bus */
        status = libFunc->SelectBus(FM_PLAT_GET_SWITCH_CFG(sw)->swNum, 
                                    FM_PLAT_BUS_XCVR_STATE, 
                                    hwResIdList[0]);
        if (status)
        {
            MOD_STATE_DEBUG("Switch %d: Failed to select transceiver bus. %s\n",
                            sw, 
                            fmErrorMsg(status) );

            DROP_PLAT_I2C_BUS_LOCK(sw);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }
    }

    /* Get transceiver state */
    status = libFunc->GetPortXcvrState(FM_PLAT_GET_SWITCH_CFG(sw)->swNum,
                                       hwResIdList,
                                       numPorts,
                                       xcvrStateValidList,
                                       xcvrStateList);
    DROP_PLAT_I2C_BUS_LOCK(sw);

    if (status)
    {
        MOD_STATE_DEBUG("Switch %d: Failed to read transceiver state. %s\n",
                        sw, 
                        fmErrorMsg(status) );
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    /**************************************************
     * Read the module EEPROM for port having a 
     * module or cable present.
     **************************************************/

    for (hwResIdIdx = 0 ; hwResIdIdx < numPorts ; hwResIdIdx++)
    {
        portIdx = hwResIdIdxList[hwResIdIdx];

        portCfg = FM_PLAT_GET_PORT_CFG(sw, portIdx);
        xcvrInfo = &GET_PLAT_STATE(sw)->xcvrInfo[portIdx];

        xcvrInfo->modState = xcvrStateList[hwResIdIdx];

        if ( (xcvrStateValidList[hwResIdIdx] & FM_PLAT_XCVR_PRESENT) &&
             (xcvrInfo->modState & FM_PLAT_XCVR_PRESENT) )
        {
            MOD_STATE_DEBUG("Port %d:%d module PRESENT\n", sw, portCfg->port);

            xcvrInfo->present = TRUE;
        }
        else
        {
            MOD_STATE_DEBUG("Port %d:%d module NOT_PRESENT\n", 
                            sw, 
                            portCfg->port);
        }

        if ( (xcvrStateValidList[hwResIdIdx] & FM_PLAT_XCVR_ENABLE) &&
             (xcvrInfo->modState & FM_PLAT_XCVR_ENABLE) )
        {
            MOD_STATE_DEBUG("Port %d:%d module is ENABLED\n", sw, portCfg->port);
 
            /* Read the module EEPROM */
            if (xcvrInfo->present)
            {
                XcvrReadAndValidateEeprom(sw, portIdx, FALSE);
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformMgmtXcvrInitialize */




/*****************************************************************************/
/* fmPlatformMgmtEnableInterrupt
 * \ingroup intPlatform
 *
 * \desc            Enable mgmt interrupt and tasks.
 *                  This assumes the switch is alive.
 *
 * \param[in]       sw is the switch number
 *
 * \return          None.
 *
 *****************************************************************************/
void fmPlatformMgmtEnableInterrupt(fm_int sw)
{
    fm_platformCfgPort *portCfg;
    fm_platformLib *    libFunc;
    fm_status           status;
    fm_uint32           hwResIdList[MAX_TEMP_PORTS];
    fm_bool             enable[MAX_TEMP_PORTS];
    fm_int              swNum;
    fm_int              portIdx;
    fm_int              numPorts;
    fm_int              gpio;

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    if ( libFunc->EnablePortIntr )
    {
        MOD_INTR_DEBUG("Switch %d: Enable mgmt interrupt in library\n", sw);

        swNum   = FM_PLAT_GET_SWITCH_CFG(sw)->swNum;
        numPorts = 0;

        for (portIdx = 0 ; portIdx < FM_PLAT_NUM_PORT(sw) ; portIdx++)
        {
            portCfg = FM_PLAT_GET_PORT_CFG(sw, portIdx);

            if ( !(portCfg->intfType == FM_PLAT_INTF_TYPE_SFPP ||
                   portCfg->intfType == FM_PLAT_INTF_TYPE_QSFP_LANE0) )
            {
                continue;
            }

            if (numPorts >= MAX_TEMP_PORTS)
            {
                break;
            }

            hwResIdList[numPorts] = portCfg->hwResourceId;
            enable[numPorts]       = TRUE;
            numPorts++;
        }

        status = libFunc->EnablePortIntr(swNum, hwResIdList, numPorts, enable);

        if (status)
        {
            FM_LOG_ERROR( FM_LOG_CAT_PLATFORM,
                         "Switch %d: %s: Failed to enable port interrupt\n",
                         sw, fmErrorMsg(status) );
        }
    }

    gpio = FM_PLAT_GET_SWITCH_CFG(sw)->gpioPortIntr;
    if ( gpio != FM_PLAT_UNDEFINED )
    {
        MOD_INTR_DEBUG("Switch %d: set GPIO %d interrupt handler\n", sw, gpio);

        /* Set up the GPIO used for mgmt interrupt */
        fmPlatformGpioSetDirection(sw, gpio, FM_PLAT_GPIO_DIR_INPUT, 0);
        fmPlatformGpioUnmaskIntr(sw, gpio, FM_PLAT_GPIO_INTR_FALLING);
    }

    /* Update the start up state */
    XcvrUpdateState( sw, TRUE, FALSE );

    enableMgmt = TRUE;

}   /* end fmPlatformMgmtEnableInterrupt */




/*****************************************************************************/
/* fmPlatformMgmtSignalInterrupt
 * \ingroup intPlatform
 *
 * \desc            Called when there is a mgmt interrupt pending.
 *
 * \param[in]       sw is the switch number
 *
 * \param[in]       enable specifies whether to reenable switch interrupt
 *                  after done processing interrupt.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmPlatformMgmtSignalInterrupt(fm_int sw, fm_int gpio)
{
    MOD_INTR_DEBUG("Switch %d: Got a mgmt interrupt (gpio %d)\n", sw, gpio);

    /* Re-enable interrupt on that GPIO */
    fmPlatformGpioUnmaskIntr(sw, gpio, FM_PLAT_GPIO_INTR_FALLING);

    fmSignalSemaphore(&mgmtSem);

}   /* end fmPlatformMgmtSignalInterrupt */




/*****************************************************************************/
/* fmPlatformMgmtSignalPollingThread
 * \ingroup intPlatform
 *
 * \desc            Signal the polling thread to run right away.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmPlatformMgmtSignalPollingThread(void)
{

    fmSignalSemaphore(&mgmtSem);
    pollingPendingTask = TRUE;

}   /* end fmPlatformMgmtSignalPollingThread */




/*****************************************************************************/
/** fmPlatformMgmtGetTransceiverType
 * \ingroup intPlatform
 *
 * \desc            Returns the transceiver type for a given port along with
 *                  length if applicable.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[out]      xcvrType points to storage where the transceiver type
 *                  will be written.
 *
 * \param[out]      xcvrLen points to storage where the transceiver cable length,
 *                  if applicable, will be written.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformMgmtGetTransceiverType(fm_int               sw,
                                           fm_int               port,
                                           fm_platformXcvrType *xcvrType,
                                           fm_int *             xcvrLen)
{
    fm_platformCfgPort *portCfg;
    fm_platXcvrInfo *   xcvrInfo;
    fm_int              portIdx;

    portIdx = fmPlatformCfgPortGetIndex(sw, port);

    if (portIdx < 0)
    {
        return FM_ERR_INVALID_PORT;
    }

    portCfg = FM_PLAT_GET_PORT_CFG(sw, portIdx);

    if ( portCfg->intfType == FM_PLAT_INTF_TYPE_QSFP_LANE1 ||
         portCfg->intfType == FM_PLAT_INTF_TYPE_QSFP_LANE2 ||
         portCfg->intfType == FM_PLAT_INTF_TYPE_QSFP_LANE3 )
    {
        /* EEPROM is read only on QSFP_LANE0 port, so get the information
           from that port */
        portIdx = 
            FM_PLAT_GET_SWITCH_CFG(sw)->epls[portCfg->epl].laneToPortIdx[0];
    }

    xcvrInfo = &GET_PLAT_STATE(sw)->xcvrInfo[portIdx];

    if (xcvrType)
    {
        *xcvrType = xcvrInfo->type;
    }

    if (xcvrLen)
    {
        *xcvrLen  = xcvrInfo->cableLength;
    }

    MOD_TYPE_DEBUG("Port %d:%d Transceiver type: %s length: %d\n",
                   sw,
                   port, 
                   fmPlatformXcvrTypeGetName(xcvrInfo->type), 
                   xcvrInfo->cableLength);

    return FM_OK;

}   /* end fmPlatformMgmtGetTransceiverType */




/*****************************************************************************/
/** fmPlatformMgmtNotifyEthModeChange
 * \ingroup intPlatform
 *
 * \desc            Called when ethernet mode change. This is used to save
 *                  the port ethernet mode for mgmt internal use.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[in]       mode is the new ethernet mode applied to the port.
 *
 * \return          NONE
 *
 *****************************************************************************/
void fmPlatformMgmtNotifyEthModeChange(fm_int     sw,
                                       fm_int     port,
                                       fm_ethMode mode)
{
    fm_int              portIdx;
    fm_platformCfgPort *portCfg;
    fm_platXcvrInfo *   xcvrInfo;

    portIdx = fmPlatformCfgPortGetIndex(sw, port);

    if (portIdx < 0)
    {
        /* Invalid port */
        return;
    }

    portCfg  = FM_PLAT_GET_PORT_CFG(sw, portIdx);
    xcvrInfo = &GET_PLAT_STATE(sw)->xcvrInfo[portIdx];

    MOD_TYPE_DEBUG("%s: port %d mode change from %s ",
                   __func__,
                   port,
                   fmPlatformGetEthModeStr(xcvrInfo->ethMode));

    MOD_TYPE_DEBUG("to %s\n",fmPlatformGetEthModeStr(mode));

    xcvrInfo->ethMode = mode;
    if (mode & FM_ETH_MODE_MULTI_LANE_MASK)
    {
        /* Multi-lane eth mode */
        fmPlatformSetPortSerdesTxCfg(sw, port, TRUE, mode);
    }
    else
    {
       /* Single lane eth mode */
        fmPlatformSetPortSerdesTxCfg(sw, port, FALSE, mode);

        if (portCfg->intfType == FM_PLAT_INTF_TYPE_SFPP &&
            xcvrInfo->present)
        {
            MOD_STATE_DEBUG("Port %d:%d EthMode change config SFP+\n", sw, port);

            if (FM_PLAT_GET_SWITCH_CFG(sw)->xcvrPollPeriodMsec == 0)
            {
                /* No background thread, so do it directly */
                ConfigureSfppXcvr(sw, portCfg->port, xcvrInfo);
            }
            else
            {
                /* Let the background thread do it */
                xcvrInfo->configRetries = MAX_CONFIG_RETRY;
                fmPlatformMgmtSignalPollingThread();
            }
        }
    }

}   /* end fmPlatformMgmtNotifyEthModeChange */



/*****************************************************************************/
/* fmPlatformMgmtConfigSfppXcvrAutoNeg
 * \ingroup intPlatform
 *
 * \desc            Perform any configuration to the SFP+ transceiver 
 *                  for autoneg.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the logical port number.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/

fm_status fmPlatformMgmtConfigSfppXcvrAutoNeg(fm_int sw,
                                              fm_int port,
                                              fm_bool enable)
{
    fm_int              portIdx;
    fm_platXcvrInfo *   xcvrInfo;

    MOD_STATE_DEBUG("Port %d:%d Config SFP+ Autoneg %d\n", sw, port, enable);

    if (FM_PLAT_GET_SWITCH_CFG(sw)->xcvrPollPeriodMsec == 0)
    {
        return FM_ERR_UNSUPPORTED;
    }

    portIdx = fmPlatformCfgPortGetIndex(sw, port);

    if (portIdx < 0)
    {
        FM_LOG_PRINT("Invalid switch %d: port %d\n", sw, port);
        return FM_ERR_INVALID_ARGUMENT;
    }

    xcvrInfo = &GET_PLAT_STATE(sw)->xcvrInfo[portIdx];

    /* Do the update in the back ground */
    xcvrInfo->configRetries = MAX_CONFIG_RETRY;
    fmPlatformMgmtSignalPollingThread();

    return FM_OK;

}   /* end fmPlatformMgmtConfigSfppXcvrAutoNeg */




/*****************************************************************************/
/** fmPlatformMgmtDumpPort
 * \ingroup intPlatform
 *
 * \desc            Dump various mgmt debug info for a given port
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the logical port number.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformMgmtDumpPort(fm_int sw, fm_int port)
{
    fm_int              portIdx;
    fm_platformCfgPort *portCfg;
    fm_platXcvrInfo *   xcvrInfo;

    portIdx = fmPlatformCfgPortGetIndex(sw, port);

    if (portIdx < 0)
    {
        FM_LOG_PRINT("Invalid switch %d: port %d\n", sw, port);
        return FM_ERR_INVALID_ARGUMENT;
    }

    portCfg  = FM_PLAT_GET_PORT_CFG(sw, portIdx);
    xcvrInfo = &GET_PLAT_STATE(sw)->xcvrInfo[portIdx];

    FM_LOG_PRINT("Switch %d: Port %d\n", sw, port);

    FM_LOG_PRINT("ethMode        : %s\n", fmPlatformGetEthModeStr(xcvrInfo->ethMode));
    FM_LOG_PRINT("disabled       : %d\n", xcvrInfo->disabled);
    FM_LOG_PRINT("anEnabled      : %d\n", xcvrInfo->anEnabled);
    FM_LOG_PRINT("TransceiverType: %s\n", 
                  fmPlatformXcvrTypeGetName(xcvrInfo->type));
    FM_LOG_PRINT("cableLength    : %d\n", xcvrInfo->cableLength);
    FM_LOG_PRINT("modSate        : 0x%x\n", xcvrInfo->modState);
    FM_LOG_PRINT("present        : %d\n", xcvrInfo->present);
    FM_LOG_PRINT("eepromBaseValid: %d\n", xcvrInfo->eepromBaseValid);
    FM_LOG_PRINT("eepromExtValid : %d\n", xcvrInfo->eepromExtValid);
    FM_LOG_PRINT("Cached EEPROM  :\n");
    fmPlatformHexDump(0, xcvrInfo->eeprom, XCVR_EEPROM_CACHE_SIZE);

    return FM_OK;

}   /* end fmPlatformMgmtDumpPort */
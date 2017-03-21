/*
 *************************************************************************
 * Ralink Tech Inc.
 * 5F., No.36, Taiyuan St., Jhubei City,
 * Hsinchu County 302,
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2010, Ralink Technology, Inc.
 *
 * This program is free software; you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation; either version 2 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 * This program is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have received a copy of the GNU General Public License     *
 * along with this program; if not, write to the                         *
 * Free Software Foundation, Inc.,                                       *
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                       *
 *************************************************************************/


#include "rt_config.h"


/*
	========================================================================
	
	Routine Description:
		Calculates the duration which is required to transmit out frames
	with given size and specified rate.
					  
	Arguments:
		pTxWI		Pointer to head of each MPDU to HW.
		Ack 		Setting for Ack requirement bit
		Fragment	Setting for Fragment bit
		RetryMode	Setting for retry mode
		Ifs 		Setting for IFS gap
		Rate		Setting for transmit rate
		Service 	Setting for service
		Length		Frame length
		TxPreamble	Short or Long preamble when using CCK rates
		QueIdx - 0-3, according to 802.11e/d4.4 June/2003
		
	Return Value:
		None
	
	See also : BASmartHardTransmit()    !!!
	
	========================================================================
*/
VOID RTMPWriteTxWI(
	IN RTMP_ADAPTER *pAd,
	IN TXWI_STRUC *pOutTxWI,
	IN BOOLEAN FRAG,
	IN BOOLEAN CFACK,
	IN BOOLEAN InsTimestamp,
	IN BOOLEAN AMPDU,
	IN BOOLEAN Ack,
	IN BOOLEAN NSeq,		/* HW new a sequence.*/
	IN UCHAR BASize,
	IN UCHAR WCID,
	IN ULONG Length,
	IN UCHAR PID,
	IN UCHAR TID,
	IN UCHAR TxRate,
	IN UCHAR Txopmode,
	IN BOOLEAN CfAck,
	IN HTTRANSMIT_SETTING *pTransmit)
{
	PMAC_TABLE_ENTRY pMac = NULL;
	TXWI_STRUC TxWI, *pTxWI;
	UINT8 TXWISize = pAd->chipCap.TXWISize;

	if (WCID < MAX_LEN_OF_MAC_TABLE)
		pMac = &pAd->MacTab.Content[WCID];

	
	/* 
		Always use Long preamble before verifiation short preamble functionality works well.
		Todo: remove the following line if short preamble functionality works
	*/
	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_SHORT_PREAMBLE_INUSED);
	NdisZeroMemory(&TxWI, TXWISize);
	pTxWI = &TxWI;
	pTxWI->TxWIFRAG= FRAG;
	pTxWI->TxWICFACK = CFACK;
	pTxWI->TxWITS= InsTimestamp;
	pTxWI->TxWIAMPDU = AMPDU;
	pTxWI->TxWIACK = Ack;
	pTxWI->TxWITXOP= Txopmode;
	
	pTxWI->TxWINSEQ = NSeq;
	/* John tune the performace with Intel Client in 20 MHz performance*/
#ifdef DOT11_N_SUPPORT
	BASize = pAd->CommonCfg.TxBASize;
#ifdef RT65xx
	if (IS_RT65XX(pAd))
	{
		if (BASize > 31)
			BASize =31;
	}
	else
#endif /* RT65xx */
	if (pAd->MACVersion == 0x28720200)
	{
		if (BASize > 13)
			BASize =13;
	}
	else
	{
		if( BASize >7 )
			BASize =7;
	}

	pTxWI->TxWIBAWinSize = BASize;
	pTxWI->TxWIShortGI = pTransmit->field.ShortGI;
	pTxWI->TxWISTBC = pTransmit->field.STBC;

#ifdef TXBF_SUPPORT
	if (pMac && pAd->chipCap.FlgHwTxBfCap)
	{
		if (pMac->TxSndgType == SNDG_TYPE_NDP  || pMac->TxSndgType == SNDG_TYPE_SOUNDING || pTxWI->eTxBF)
			pTxWI->TxWISTBC = 0;
	}
#endif /* TXBF_SUPPORT */
#endif /* DOT11_N_SUPPORT */
		
	pTxWI->TxWIWirelessCliID = WCID;
	pTxWI->TxWIMPDUByteCnt = Length;
	pTxWI->TxWIPacketId = PID;
	
	/* If CCK or OFDM, BW must be 20*/
	pTxWI->TxWIBW = (pTransmit->field.MODE <= MODE_OFDM) ? (BW_20) : (pTransmit->field.BW);
#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3
	if (pTxWI->TxWIBW)
		pTxWI->TxWIBW = (pAd->CommonCfg.AddHTInfo.AddHtInfo.RecomWidth == 0) ? (BW_20) : (pTransmit->field.BW);
#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */
	
	pTxWI->TxWIMCS = pTransmit->field.MCS;
	pTxWI->TxWIPHYMODE = pTransmit->field.MODE;
	pTxWI->TxWICFACK = CfAck;

#ifdef DOT11_N_SUPPORT
	if (pMac)
	{
        if (pAd->CommonCfg.bMIMOPSEnable)
        {
    		if ((pMac->MmpsMode == MMPS_DYNAMIC) && (pTransmit->field.MCS > 7))
			{
				/* Dynamic MIMO Power Save Mode*/
				pTxWI->TxWIMIMOps = 1;
			}
			else if (pMac->MmpsMode == MMPS_STATIC)
			{
				/* Static MIMO Power Save Mode*/
				if (pTransmit->field.MODE >= MODE_HTMIX && pTransmit->field.MCS > 7)
				{
					pTxWI->TxWIMCS = 7;
					pTxWI->TxWIMIMOps = 0;
				}
			}
        }
		/*pTxWI->TxWIMIMOps = (pMac->PsMode == PWR_MMPS)? 1:0;*/
		{
			pTxWI->TxWIMpduDensity = pMac->MpduDensity;
		}
	}
#endif /* DOT11_N_SUPPORT */


	pTxWI->TxWIPacketId = pTxWI->TxWIMCS;
	NdisMoveMemory(pOutTxWI, &TxWI, TXWISize);
//+++Add by shiang for debug
if (0){
	hex_dump("TxWI", (UCHAR *)pOutTxWI, TXWISize);
}
//---Add by shiang for debug
}


VOID RTMPWriteTxWI_Data(RTMP_ADAPTER *pAd, TXWI_STRUC *pTxWI, TX_BLK *pTxBlk)
{
	HTTRANSMIT_SETTING *pTransmit;
	MAC_TABLE_ENTRY *pMacEntry;
#ifdef DOT11_N_SUPPORT
	UCHAR BASize;
#endif /* DOT11_N_SUPPORT */
	UINT8 TXWISize = pAd->chipCap.TXWISize;


	ASSERT(pTxWI);

	pTransmit = pTxBlk->pTransmit;
	pMacEntry = pTxBlk->pMacEntry;

	/*
		Always use Long preamble before verifiation short preamble functionality works well.
		Todo: remove the following line if short preamble functionality works
	*/
	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_SHORT_PREAMBLE_INUSED);
	NdisZeroMemory(pTxWI, TXWISize);
	
	pTxWI->TxWIFRAG = TX_BLK_TEST_FLAG(pTxBlk, fTX_bAllowFrag);
	pTxWI->TxWIACK = TX_BLK_TEST_FLAG(pTxBlk, fTX_bAckRequired);
	pTxWI->TxWITXOP = pTxBlk->FrameGap;

#ifdef CONFIG_STA_SUPPORT
#ifdef QOS_DLS_SUPPORT
	if (pMacEntry && IS_ENTRY_DLS(pMacEntry) &&
		(pAd->StaCfg.BssType == BSS_INFRA))
		pTxWI->TxWIWirelessCliID = BSSID_WCID;
	else
#endif /* QOS_DLS_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */
	pTxWI->TxWIWirelessCliID = pTxBlk->Wcid;

	pTxWI->TxWIMPDUByteCnt = pTxBlk->MpduHeaderLen + pTxBlk->SrcBufLen;
	pTxWI->TxWICFACK = TX_BLK_TEST_FLAG(pTxBlk, fTX_bPiggyBack);

#ifdef WFA_VHT_PF
	if (pAd->force_noack == TRUE)
		pTxWI->TxWIACK = 0;
#endif /* WFA_VHT_PF */

	pTxWI->TxWIShortGI = pTransmit->field.ShortGI;
	pTxWI->TxWISTBC = pTransmit->field.STBC;
	pTxWI->TxWIMCS = pTransmit->field.MCS;
	pTxWI->TxWIPHYMODE = pTransmit->field.MODE;

	/* If CCK or OFDM, BW must be 20 */
	pTxWI->TxWIBW = (pTransmit->field.MODE <= MODE_OFDM) ? (BW_20) : (pTransmit->field.BW);
#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3
	if (pTxWI->TxWIBW)
		pTxWI->TxWIBW = (pAd->CommonCfg.AddHTInfo.AddHtInfo.RecomWidth == 0) ? (BW_20) : (pTransmit->field.BW);
#endif /* DOT11N_DRAFT3 */

	pTxWI->TxWIAMPDU = ((pTxBlk->TxFrameType == TX_AMPDU_FRAME) ? TRUE : FALSE);
	BASize = pAd->CommonCfg.TxBASize;
	if((pTxBlk->TxFrameType == TX_AMPDU_FRAME) && (pMacEntry))
	{
		UCHAR RABAOriIdx = pTxBlk->pMacEntry->BAOriWcidArray[pTxBlk->UserPriority];

		BASize = pAd->BATable.BAOriEntry[RABAOriIdx].BAWinSize;
	}

	pTxWI->TxWIBAWinSize = BASize;

#ifdef TXBF_SUPPORT
	if(pTxBlk->TxSndgPkt > SNDG_TYPE_DISABLE)
		pTxWI->TxWIAMPDU = FALSE;

	if (pTxBlk->TxSndgPkt == SNDG_TYPE_SOUNDING)
	{
		pTxWI->Sounding = 1;
		DBGPRINT(RT_DEBUG_TRACE, ("ETxBF in RTMPWriteTxWI_Data(): sending normal sounding, eTxBF=%d\n", pTxWI->eTxBF));
		pTxWI->iTxBF = 0;
	}
	else if (pTxBlk->TxSndgPkt == SNDG_TYPE_NDP)
	{
		if (pTxBlk->TxNDPSndgMcs >= 16)
			pTxWI->NDPSndRate = 2;
		else if (pTxBlk->TxNDPSndgMcs >= 8)
			pTxWI->NDPSndRate = 1;
		else
			pTxWI->NDPSndRate = 0;

		pTxWI->NDPSndBW = pTransmit->field.BW;
		pTxWI->iTxBF = 0;
	}
	else
	{
#ifdef MFB_SUPPORT
		if (pMacEntry && (pMacEntry->mrqCnt >0) && (pMacEntry->toTxMrq == TRUE))
			pTxWI->eTxBF = ~(pTransmit->field.eTxBF);
		else
#endif	/* MFB_SUPPORT */
			pTxWI->eTxBF = pTransmit->field.eTxBF;
		pTxWI->iTxBF = pTransmit->field.iTxBF;
	}

	if (pTxBlk->TxSndgPkt == SNDG_TYPE_NDP  || pTxBlk->TxSndgPkt == SNDG_TYPE_SOUNDING || pTxWI->eTxBF)
		pTxWI->TxWISTBC = 0;
#endif /* TXBF_SUPPORT */

#endif /* DOT11_N_SUPPORT */


#ifdef DOT11_N_SUPPORT
	if (pMacEntry)
	{
		if ((pMacEntry->MmpsMode == MMPS_DYNAMIC) && (pTransmit->field.MCS > 7))
		{
			/* Dynamic MIMO Power Save Mode*/
			pTxWI->TxWIMIMOps = 1;
		}
		else if (pMacEntry->MmpsMode == MMPS_STATIC)
		{
			/* Static MIMO Power Save Mode*/
			if ((pTransmit->field.MODE == MODE_HTMIX || pTransmit->field.MODE == MODE_HTGREENFIELD) && 
				(pTransmit->field.MCS > 7))
			{
				pTxWI->TxWIMCS = 7;
				pTxWI->TxWIMIMOps = 0;
			}
		}

		pTxWI->TxWIMpduDensity = pMacEntry->MpduDensity;
	}
#endif /* DOT11_N_SUPPORT */
	
#ifdef TXBF_SUPPORT
	if (pTxBlk->TxSndgPkt > SNDG_TYPE_DISABLE)
	{
		pTxWI->TxWIMCS = 0;
		pTxWI->TxWIAMPDU = FALSE;
	}
#endif /* TXBF_SUPPORT */
	
#ifdef DBG_DIAGNOSE
	if (pTxBlk->QueIdx== 0)
	{
		pAd->DiagStruct.TxDataCnt[pAd->DiagStruct.ArrayCurIdx]++;
		pAd->DiagStruct.TxMcsCnt[pAd->DiagStruct.ArrayCurIdx][pTxWI->MCS]++;
	}
#endif /* DBG_DIAGNOSE */

	/* for rate adapation*/
	pTxWI->TxWIPacketId = pTxWI->TxWIMCS;

#ifdef INF_AMAZON_SE
	/*Iverson patch for WMM A5-T07 ,WirelessStaToWirelessSta do not bulk out aggregate */
	if( RTMP_GET_PACKET_NOBULKOUT(pTxBlk->pPacket))
	{
		if(pTxWI->TxWIPHYMODE == MODE_CCK)
			pTxWI->TxWIPacketId = 6;
	}	
#endif /* INF_AMAZON_SE */	


#ifdef CONFIG_FPGA_MODE
	if (pAd->fpga_ctl.fpga_on & 0x6)
	{
		pTxWI->TxWIPHYMODE = pAd->fpga_ctl.tx_data_phy;
		pTxWI->TxWIMCS = pAd->fpga_ctl.tx_data_mcs;
		pTxWI->TxWILDPC = pAd->fpga_ctl.tx_data_ldpc; 
		pTxWI->TxWIBW = pAd->fpga_ctl.tx_data_bw;
		pTxWI->TxWIShortGI = pAd->fpga_ctl.tx_data_gi;
		if (pAd->fpga_ctl.data_basize)
			pTxWI->TxWIBAWinSize = pAd->fpga_ctl.data_basize;
	}
#endif /* CONFIG_FPGA_MODE */

#ifdef MCS_LUT_SUPPORT
	if ((RTMP_TEST_MORE_FLAG(pAd, fASIC_CAP_MCS_LUT)) && 
		(pTxWI->TxWIWirelessCliID < 128) && 
		(pMacEntry && pMacEntry->bAutoTxRateSwitch == TRUE))
	{
		HTTRANSMIT_SETTING rate_ctrl;

		rate_ctrl.field.MODE = pTxWI->TxWIPHYMODE;
#ifdef TXBF_SUPPORT
		rate_ctrl.field.iTxBF = pTxWI->iTxBF;
		rate_ctrl.field.eTxBF = pTxWI->eTxBF;
#endif /* TXBF_SUPPORT */
		rate_ctrl.field.STBC = pTxWI->TxWISTBC;
		rate_ctrl.field.ShortGI = pTxWI->TxWIShortGI;
		rate_ctrl.field.BW = pTxWI->TxWIBW;
		rate_ctrl.field.MCS = pTxWI->TxWIMCS; 
		if (rate_ctrl.word == pTransmit->word)
			pTxWI->TxWILutEn = 1;
		pTxWI->TxWILutEn = 0;
	}
#endif /* MCS_LUT_SUPPORT */

}


VOID RTMPWriteTxWI_Cache(
	IN RTMP_ADAPTER *pAd,
	INOUT TXWI_STRUC *pTxWI,
	IN TX_BLK *pTxBlk)
{
	HTTRANSMIT_SETTING *pTransmit;
	MAC_TABLE_ENTRY *pMacEntry;
#ifdef DOT11_N_SUPPORT
#endif /* DOT11_N_SUPPORT */
	
	
	/* update TXWI */
	pMacEntry = pTxBlk->pMacEntry;
	pTransmit = pTxBlk->pTransmit;
	
	if (pMacEntry->bAutoTxRateSwitch)
	{
		pTxWI->TxWITXOP = IFS_HTTXOP;

		/* If CCK or OFDM, BW must be 20*/
		pTxWI->TxWIBW = (pTransmit->field.MODE <= MODE_OFDM) ? (BW_20) : (pTransmit->field.BW);
		pTxWI->TxWIShortGI = pTransmit->field.ShortGI;
		pTxWI->TxWISTBC = pTransmit->field.STBC;

#ifdef TXBF_SUPPORT
		if (pTxBlk->TxSndgPkt == SNDG_TYPE_NDP  || pTxBlk->TxSndgPkt == SNDG_TYPE_SOUNDING || pTxWI->eTxBF)
			pTxWI->TxWISTBC = 0;
#endif /* TXBF_SUPPORT */

		pTxWI->TxWIMCS = pTransmit->field.MCS;
		pTxWI->TxWIPHYMODE = pTransmit->field.MODE;

		/* set PID for TxRateSwitching*/
		pTxWI->TxWIPacketId = pTransmit->field.MCS;
		
	}

#ifdef DOT11_N_SUPPORT
	pTxWI->TxWIAMPDU = ((pMacEntry->NoBADataCountDown == 0) ? TRUE: FALSE);
#ifdef TXBF_SUPPORT
	if(pTxBlk->TxSndgPkt > SNDG_TYPE_DISABLE)
		pTxWI->TxWIAMPDU = FALSE;
#endif /* TXBF_SUPPORT */

	pTxWI->TxWIMIMOps = 0;

#ifdef DOT11N_DRAFT3
	if (pTxWI->TxWIBW)
		pTxWI->TxWIBW = (pAd->CommonCfg.AddHTInfo.AddHtInfo.RecomWidth == 0) ? (BW_20) : (pTransmit->field.BW);
#endif /* DOT11N_DRAFT3 */

    if (pAd->CommonCfg.bMIMOPSEnable)
    {
		/* MIMO Power Save Mode*/
		if ((pMacEntry->MmpsMode == MMPS_DYNAMIC) && (pTransmit->field.MCS > 7))
		{
			/* Dynamic MIMO Power Save Mode*/
			pTxWI->TxWIMIMOps = 1;
		}
		else if (pMacEntry->MmpsMode == MMPS_STATIC)
		{
			/* Static MIMO Power Save Mode*/
			if ((pTransmit->field.MODE >= MODE_HTMIX) && (pTransmit->field.MCS > 7))
			{
				pTxWI->TxWIMCS = 7;
				pTxWI->TxWIMIMOps = 0;
			}
		}
    }

#endif /* DOT11_N_SUPPORT */

#ifdef DBG_DIAGNOSE
	if (pTxBlk->QueIdx== 0)
	{
		pAd->DiagStruct.TxDataCnt[pAd->DiagStruct.ArrayCurIdx]++;
		pAd->DiagStruct.TxMcsCnt[pAd->DiagStruct.ArrayCurIdx][pTxWI->MCS]++;
	}
#endif /* DBG_DIAGNOSE */

#ifdef TXBF_SUPPORT
	if (pTxBlk->TxSndgPkt == SNDG_TYPE_SOUNDING)
	{
		pTxWI->Sounding = 1;
		pTxWI->eTxBF = 0;
		pTxWI->iTxBF = 0;
		DBGPRINT(RT_DEBUG_TRACE, ("ETxBF in RTMPWriteTxWI_Cache(): sending normal sounding, eTxBF=%d\n", pTxWI->eTxBF));
	}
	else if (pTxBlk->TxSndgPkt == SNDG_TYPE_NDP)
	{
		if (pTxBlk->TxNDPSndgMcs>=16)
			pTxWI->NDPSndRate = 2;
		else if (pTxBlk->TxNDPSndgMcs>=8)
			pTxWI->NDPSndRate = 1;
		else
			pTxWI->NDPSndRate = 0;
		pTxWI->Sounding = 0;
		pTxWI->eTxBF = 0;
		pTxWI->iTxBF = 0;

		pTxWI->NDPSndBW = pTransmit->field.BW;

/*
		DBGPRINT(RT_DEBUG_TRACE,
				("%s():ETxBF, sending ndp sounding(BW=%d, Rate=%d, eTxBF=%d)\n",
				__FUNCTION__, pTxWI->NDPSndBW, pTxWI->NDPSndRate, pTxWI->eTxBF));
*/
	}
	else
	{
		pTxWI->Sounding = 0;
#ifdef MFB_SUPPORT
		if (pMacEntry && pMacEntry->mrqCnt >0 && pMacEntry->toTxMrq == 1)
		{
			pTxWI->eTxBF = ~(pTransmit->field.eTxBF);
			DBGPRINT_RAW(RT_DEBUG_TRACE,("ETxBF in AP_AMPDU_Frame_Tx(): invert eTxBF\n"));
		}
		else
#endif	/* MFB_SUPPORT */
			pTxWI->eTxBF = pTransmit->field.eTxBF;

		pTxWI->iTxBF = pTransmit->field.iTxBF;

		if (pTxWI->eTxBF || pTxWI->iTxBF)
			pTxWI->TxWISTBC = 0;
	}

	if (pTxBlk->TxSndgPkt > SNDG_TYPE_DISABLE)
	{
		pTxWI->TxWIMCS = 0;
		pTxWI->TxWIAMPDU = FALSE;
	}
#endif /* TXBF_SUPPORT */

	pTxWI->TxWIMPDUByteCnt = pTxBlk->MpduHeaderLen + pTxBlk->SrcBufLen;


#ifdef WFA_VHT_PF
	if (pAd->force_noack == TRUE)
		pTxWI->TxWIACK = 0;
	else
#endif /* WFA_VHT_PF */
		pTxWI->TxWIACK = TX_BLK_TEST_FLAG(pTxBlk, fTX_bAckRequired);

#ifdef CONFIG_FPGA_MODE
	if (pAd->fpga_ctl.fpga_on & 0x6)
	{
		pTxWI->TxWIPHYMODE = pAd->fpga_ctl.tx_data_phy;
		pTxWI->TxWIMCS = pAd->fpga_ctl.tx_data_mcs;
		pTxWI->TxWIBW = pAd->fpga_ctl.tx_data_bw;
		pTxWI->TxWIShortGI = pAd->fpga_ctl.tx_data_gi;
		if (pAd->fpga_ctl.data_basize)
			pTxWI->TxWIBAWinSize = pAd->fpga_ctl.data_basize;
	}
#endif /* CONFIG_FPGA_MODE */

#ifdef MCS_LUT_SUPPORT
	if (RTMP_TEST_MORE_FLAG(pAd, fASIC_CAP_MCS_LUT) && 
		(pTxWI->TxWIWirelessCliID < 128) && 
		(pMacEntry && pMacEntry->bAutoTxRateSwitch == TRUE))
	{
		HTTRANSMIT_SETTING rate_ctrl;
		
		rate_ctrl.field.MODE = pTxWI->TxWIPHYMODE;
#ifdef TXBF_SUPPORT
		rate_ctrl.field.iTxBF = pTxWI->iTxBF;
		rate_ctrl.field.eTxBF = pTxWI->eTxBF;
#endif /* TXBF_SUPPORT */
		rate_ctrl.field.STBC = pTxWI->TxWISTBC;
		rate_ctrl.field.ShortGI = pTxWI->TxWIShortGI;
		rate_ctrl.field.BW = pTxWI->TxWIBW;
		rate_ctrl.field.MCS = pTxWI->TxWIMCS; 
		if (rate_ctrl.word == pTransmit->word)
			pTxWI->TxWILutEn = 1;
		pTxWI->TxWILutEn = 0;
	}
#endif /* MCS_LUT_SUPPORT */

}


INT rtmp_mac_set_band(RTMP_ADAPTER *pAd, int  band)
{
	UINT32 val, band_cfg;


	RTMP_IO_READ32(pAd, TX_BAND_CFG, &band_cfg);
	val = band_cfg & (~0x6);
	switch (band)
	{
		case BAND_5G:
			val |= 0x02;
			break;
		case BAND_24G:
		default:
			val |= 0x4;
			break;
	}

	if (val != band_cfg)
		RTMP_IO_WRITE32(pAd, TX_BAND_CFG, val);

	return TRUE;
}


INT rtmp_mac_set_ctrlch(RTMP_ADAPTER *pAd, INT extch)
{
	UINT32 val, band_cfg;


	RTMP_IO_READ32(pAd, TX_BAND_CFG, &band_cfg);
	val = band_cfg & (~0x1);
	switch (extch)
	{
		case EXTCHA_ABOVE:
			val &= (~0x1);
			break;
		case EXTCHA_BELOW:
			val |= (0x1);
			break;
		case EXTCHA_NONE:
			val &= (~0x1);
			break;
	}

	if (val != band_cfg)
		RTMP_IO_WRITE32(pAd, TX_BAND_CFG, val);
	
	return TRUE;
}


INT rtmp_mac_set_mmps(RTMP_ADAPTER *pAd, INT ReduceCorePower)
{
	UINT32 mac_val, org_val;

	RTMP_IO_READ32(pAd, 0x1210, &org_val);
	mac_val = org_val;
	if (ReduceCorePower)
		mac_val |= 0x09;
	else
		mac_val &= ~0x09;

	if (mac_val != org_val)
		RTMP_IO_WRITE32(pAd, 0x1210, mac_val);

	return TRUE;
}


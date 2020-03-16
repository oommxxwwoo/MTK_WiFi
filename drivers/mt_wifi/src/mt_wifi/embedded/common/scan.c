/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2004, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:

	Abstract:

	Revision History:
	Who		When			What
	--------	----------		----------------------------------------------
*/

#include "rt_config.h"

#ifdef SCAN_SUPPORT


INT scan_ch_restore(RTMP_ADAPTER *pAd, UCHAR OpMode, struct wifi_dev *pwdev)
{
	INT bw, ch;
	struct wifi_dev *wdev = pwdev;
#ifdef APCLI_SUPPORT
#ifdef APCLI_CERT_SUPPORT
	UCHAR  ScanType = pAd->ScanCtrl.ScanType;
#endif /*APCLI_CERT_SUPPORT*/
#endif /*APCLI_SUPPORT*/
#ifdef CONFIG_AP_SUPPORT
	BSS_STRUCT *pMbss = NULL;
	struct DOT11_H *pDot11h = NULL;
#endif
#ifdef CONFIG_MULTI_CHANNEL
#if defined(RT_CFG80211_SUPPORT) && defined(CONFIG_AP_SUPPORT)
	pMbss = &pAd->ApCfg.MBSSID[CFG_GO_BSSID_IDX];
	PAPCLI_STRUCT pApCliEntry = pApCliEntry = &pAd->ApCfg.ApCliTab[MAIN_MBSSID];
	struct wifi_dev *p2p_wdev = &pMbss->wdev;

	if (RTMP_CFG80211_VIF_P2P_GO_ON(pAd))
		p2p_wdev = &pMbss->wdev;
	else if (RTMP_CFG80211_VIF_P2P_CLI_ON(pAd))
		p2p_wdev = &pApCliEntry->wdev;

#endif /* defined(RT_CFG80211_SUPPORT) && defined(CONFIG_AP_SUPPORT) */
#endif /* CONFIG_MULTI_CHANNEL */
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		if (!wdev) {
			pMbss = &pAd->ApCfg.MBSSID[MAIN_MBSSID];
			wdev = &pMbss->wdev;
		}
	}
#endif /*CONFIG_STA_SUPPORT*/
	ch = wlan_operate_get_cen_ch_1(wdev);
	bw = wlan_operate_get_bw(wdev);

#ifdef CONFIG_MULTI_CHANNEL
#if defined(RT_CFG80211_SUPPORT) && defined(CONFIG_AP_SUPPORT)

	if (RTMP_CFG80211_VIF_P2P_GO_ON(pAd) && (ch != p2p_wdev->channel) && (p2p_wdev->CentralChannel != 0))
		bw = wlan_operate_get_ht_bw(p2p_wdev);
	else if (RTMP_CFG80211_VIF_P2P_CLI_ON(pAd) && (ch != p2p_wdev->channel) && (p2p_wdev->CentralChannel != 0))
		bw = wlan_operate_get_ht_bw(p2p_wdev);

#endif /* defined(RT_CFG80211_SUPPORT) && defined(CONFIG_AP_SUPPORT) */
#endif /* CONFIG_MULTI_CHANNEL */
#ifdef CONFIG_MULTI_CHANNEL
#if defined(RT_CFG80211_SUPPORT) && defined(CONFIG_AP_SUPPORT)
	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("scan ch restore   ch %d  p2p_wdev->CentralChannel%d\n", ch, p2p_wdev->CentralChannel));

	/*If GO start, we need to change to GO Channel*/
	if ((ch != p2p_wdev->CentralChannel) && (p2p_wdev->CentralChannel != 0))
		ch = p2p_wdev->CentralChannel;

#endif /* defined(RT_CFG80211_SUPPORT) && defined(CONFIG_AP_SUPPORT) */
#endif /* CONFIG_MULTI_CHANNEL */
	ch = wlan_operate_get_prim_ch(wdev);
	ASSERT((ch != 0));
	/*restore to original channel*/
	wlan_operate_set_prim_ch(wdev, ch);
	ch = wlan_operate_get_cen_ch_1(wdev);
	bw = wlan_operate_get_bw(wdev);
	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s,central ch=%d,bw=%d\n\r", __func__, ch, bw));
	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("SYNC - End of SCAN, restore to %dMHz channel %d, Total BSS[%02d]\n",
			 bw, ch, pAd->ScanTab.BssNr));
#ifdef CONFIG_AP_SUPPORT

	if (OpMode == OPMODE_AP) {
		INT BssIdx;
		INT MaxNumBss = pAd->ApCfg.BssidNum;
#ifdef APCLI_SUPPORT
		if (wdev && wdev->wdev_type == WDEV_TYPE_APCLI) {
#ifdef APCLI_AUTO_CONNECT_SUPPORT
			if (pAd->ApCfg.ApCliAutoConnectRunning[wdev->func_idx] == TRUE &&
				pAd->ScanCtrl.PartialScan.bScanning == FALSE) {
				if (!ApCliAutoConnectExec(pAd, wdev))
					MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						("Error in  %s\n", __func__));
			}
#endif /* APCLI_AUTO_CONNECT_SUPPORT */
			pAd->ApCfg.bPartialScanning[wdev->func_idx] = FALSE;
		}
#endif /* APCLI_SUPPORT */
		pAd->Mlme.ApSyncMachine.CurrState = AP_SYNC_IDLE;


		/* iwpriv set auto channel selection*/
		/* scanned all channels*/
		if (wdev == NULL)
			return FALSE;

		pDot11h = wdev->pDot11_H;
		if (pDot11h == NULL)
			return FALSE;

		if (((wdev->channel > 14) &&
			 (pAd->CommonCfg.bIEEE80211H == TRUE) &&
			 RadarChannelCheck(pAd, wdev->channel)) &&
			pDot11h->RDMode != RD_SWITCHING_MODE) {
			if (pDot11h->InServiceMonitorCount) {
				pDot11h->RDMode = RD_NORMAL_MODE;
				AsicSetSyncModeAndEnable(pAd, pAd->CommonCfg.BeaconPeriod, HW_BSSID_0, OPMODE_AP);

				/* Enable beacon tx for all BSS */
				for (BssIdx = 0; BssIdx < MaxNumBss; BssIdx++) {
					wdev = &pAd->ApCfg.MBSSID[BssIdx].wdev;

					if (wdev->bAllowBeaconing)
						UpdateBeaconHandler(pAd, wdev, BCN_UPDATE_ENABLE_TX);
				}
			} else
				pDot11h->RDMode = RD_SILENCE_MODE;
		} else {
			AsicSetSyncModeAndEnable(pAd, pAd->CommonCfg.BeaconPeriod, HW_BSSID_0, OPMODE_AP);

			/* Enable beacon tx for all BSS */
			for (BssIdx = 0; BssIdx < MaxNumBss; BssIdx++) {
				wdev = &pAd->ApCfg.MBSSID[BssIdx].wdev;

				if (wdev->bAllowBeaconing)
					UpdateBeaconHandler(pAd, wdev, BCN_UPDATE_ENABLE_TX);
			}
		}

#ifdef APCLI_SUPPORT
#ifdef WSC_AP_SUPPORT
		if (wdev &&
#ifdef CON_WPS
			/* In case of concurrent WPS, the request might have come from a non APCLI interface. */
			((wdev->wdev_type == WDEV_TYPE_APCLI) || (pAd->ApCfg.ApCliTab[wdev->func_idx].wdev.WscControl.conWscStatus != CON_WPS_STATUS_DISABLED)) &&
#else
			(wdev->wdev_type == WDEV_TYPE_APCLI) &&

#endif
			(wdev->func_idx < MAX_APCLI_NUM)) {
			WSC_CTRL *pWpsCtrlTemp = &pAd->ApCfg.ApCliTab[wdev->func_idx].wdev.WscControl;

			if ((pWpsCtrlTemp->WscConfMode != WSC_DISABLE) &&
				(pWpsCtrlTemp->bWscTrigger == TRUE) &&
				(pWpsCtrlTemp->WscMode == WSC_PBC_MODE)) {
				if (pWpsCtrlTemp->WscApCliScanMode == TRIGGER_PARTIAL_SCAN) {
					if ((pAd->ScanCtrl.PartialScan.bScanning == FALSE) &&
						(pAd->ScanCtrl.PartialScan.LastScanChannel == 0)) {
						MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
							("[%s] %s AP-Client WPS Partial Scan done!!!\n",
							__func__, (ch > 14 ? "5G" : "2G")));

#ifdef CON_WPS
						if (pWpsCtrlTemp->conWscStatus != CON_WPS_STATUS_DISABLED) {
							MlmeEnqueue(pAd, AP_SYNC_STATE_MACHINE,
								 APMT2_MLME_SCAN_COMPLETE, 0, NULL, wdev->func_idx);
							RTMP_MLME_HANDLER(pAd);
						} else
#endif /* CON_WPS */
						{
							if (!pWpsCtrlTemp->WscPBCTimerRunning) {
								RTMPSetTimer(&pWpsCtrlTemp->WscPBCTimer, 1000);
								pWpsCtrlTemp->WscPBCTimerRunning = TRUE;
							}
						}
					}
				}
#ifdef CON_WPS
				else {
					if (pWpsCtrlTemp->conWscStatus != CON_WPS_STATUS_DISABLED) {
						MlmeEnqueue(pAd, AP_SYNC_STATE_MACHINE, APMT2_MLME_SCAN_COMPLETE, 0,
								NULL, wdev->func_idx);
						RTMP_MLME_HANDLER(pAd);
					}
				}
#endif /* CON_WPS*/
			}
		}
#endif /* WSC_AP_SUPPORT */
#endif /* APCLI_SUPPORT */
	}

#ifdef APCLI_SUPPORT
#ifdef APCLI_CERT_SUPPORT
#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3
	{
		UCHAR apcli2Gidx = 0;
#ifdef DBDC_MODE
		if (pAd->CommonCfg.dbdc_mode)
			apcli2Gidx = 1;
#endif
		if ((pAd->bApCliCertTest == TRUE) && APCLI_IF_UP_CHECK(pAd, apcli2Gidx) && (ScanType == SCAN_2040_BSS_COEXIST)) {
			UCHAR Status = 1;

			MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("@(%s)  Scan Done ScanType=%d\n", __func__, ScanType));
			MlmeEnqueue(pAd, APCLI_CTRL_STATE_MACHINE, APCLI_CTRL_SCAN_DONE, 2, &Status, apcli2Gidx);
		}
	}
#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */
#endif /* APCLI_CERT_SUPPORT */
#endif /* APCLI_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
	return TRUE;
}



static INT scan_active(RTMP_ADAPTER *pAd, UCHAR OpMode, UCHAR ScanType, struct wifi_dev *wdev)
{
	UCHAR *frm_buf = NULL;
	HEADER_802_11 Hdr80211;
	ULONG FrameLen = 0;
	UCHAR SsidLen = 0;
	struct _build_ie_info ie_info;


	ie_info.frame_subtype = SUBTYPE_PROBE_REQ;
	ie_info.channel = pAd->ScanCtrl.Channel;
	ie_info.phy_mode = wdev->PhyMode;
	ie_info.wdev = wdev;

	if (MlmeAllocateMemory(pAd, &frm_buf) != NDIS_STATUS_SUCCESS) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s():allocate memory fail\n", __func__));
#ifdef CONFIG_AP_SUPPORT

		if (OpMode == OPMODE_AP)
			pAd->Mlme.ApSyncMachine.CurrState = AP_SYNC_IDLE;

#endif /* CONFIG_AP_SUPPORT */
		return FALSE;
	}

#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3

	if (ScanType == SCAN_2040_BSS_COEXIST)
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("SYNC - SCAN_2040_BSS_COEXIST !! Prepare to send Probe Request\n"));

#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */
	/* There is no need to send broadcast probe request if active scan is in effect.*/
	SsidLen = 0;

	if ((ScanType == SCAN_ACTIVE) || (ScanType == FAST_SCAN_ACTIVE)
#ifdef WSC_STA_SUPPORT
		|| ((ScanType == SCAN_WSC_ACTIVE) && (OpMode == OPMODE_STA))
#endif /* WSC_STA_SUPPORT */
	   )
		SsidLen = pAd->ScanCtrl.SsidLen;

		{
#ifdef CONFIG_AP_SUPPORT

			/*IF_DEV_CONFIG_OPMODE_ON_AP(pAd) */
			if (OpMode == OPMODE_AP) {
				UCHAR *src_mac_addr = NULL;
#ifdef APCLI_SUPPORT
#ifdef WSC_INCLUDED

				if (ScanType == SCAN_WSC_ACTIVE)
					src_mac_addr = &pAd->ApCfg.ApCliTab[wdev->func_idx].wdev.if_addr[0];
				else
#endif
#endif
				{/* search the first ap interface which use the same band */
					INT IdBss = 0;

					for (IdBss = 0; IdBss < pAd->ApCfg.BssidNum; IdBss++) {
						if (pAd->ApCfg.MBSSID[IdBss].wdev.DevInfo.Active) {
							if (HcGetBandByWdev(&pAd->ApCfg.MBSSID[IdBss].wdev) == HcGetBandByWdev(wdev))
								break;
						}
					}

					src_mac_addr = &pAd->ApCfg.MBSSID[IdBss].wdev.bssid[0];
				}

				MgtMacHeaderInitExt(pAd, &Hdr80211, SUBTYPE_PROBE_REQ, 0, BROADCAST_ADDR,
									src_mac_addr,
									BROADCAST_ADDR);
			}

#endif /* CONFIG_AP_SUPPORT */
			MakeOutgoingFrame(frm_buf,               &FrameLen,
							  sizeof(HEADER_802_11),    &Hdr80211,
							  1,                        &SsidIe,
							  1,                        &SsidLen,
							  SsidLen,			        pAd->ScanCtrl.Ssid,
							  1,                        &SupRateIe,
							  1,                        &wdev->rate.SupRateLen,
							  wdev->rate.SupRateLen,  wdev->rate.SupRate,
							  END_OF_ARGS);

			if (wdev->rate.ExtRateLen) {
				ULONG Tmp;
				MakeOutgoingFrame(frm_buf + FrameLen,            &Tmp,
								  1,                                &ExtRateIe,
								  1,                                &wdev->rate.ExtRateLen,
								  wdev->rate.ExtRateLen,          wdev->rate.ExtRate,
								  END_OF_ARGS);
				FrameLen += Tmp;
			}
		}

#ifdef DOT11_N_SUPPORT

	if (WMODE_CAP_N(wdev->PhyMode)) {
		ie_info.frame_buf = (UCHAR *)(frm_buf + FrameLen);
		ie_info.is_draft_n_type = FALSE;
		FrameLen += build_ht_ies(pAd, &ie_info);

		if (pAd->bBroadComHT == TRUE) {
			ie_info.is_draft_n_type = TRUE;
			ie_info.frame_buf = (UCHAR *)(frm_buf + FrameLen);
			FrameLen += build_ht_ies(pAd, &ie_info);
		}
#ifdef DOT11_VHT_AC
		ie_info.frame_buf = (UCHAR *)(frm_buf + FrameLen);
		FrameLen += build_vht_ies(pAd, &ie_info);
#endif /* DOT11_VHT_AC */
	}
#endif /* DOT11_N_SUPPORT */

#ifdef WSC_INCLUDED
	/* If active scan is not from WSC then don't include it */
	/* As it can cause AP to detect two probe with wsc active */
	/* and can cause WPS AP overlap */
	if (ScanType == SCAN_WSC_ACTIVE) {
		ie_info.frame_buf = (UCHAR *)(frm_buf + FrameLen);
		FrameLen += build_wsc_ie(pAd, &ie_info);
	}
#endif /* WSC_INCLUDED */

	ie_info.frame_buf = (UCHAR *)(frm_buf + FrameLen);
	FrameLen +=  build_extra_ie(pAd, &ie_info);

	MiniportMMRequest(pAd, 0, frm_buf, FrameLen);
#ifdef MT_MAC_BTCOEX

	if (pAd->BtCoexMode == MT7636_COEX_MODE_TDD)
		MiniportMMRequest(pAd, 0, frm_buf, FrameLen);

#endif
	MlmeFreeMemory(frm_buf);
	return TRUE;
}

#ifdef APCLI_SUPPORT
static void FireExtraProbeReq(RTMP_ADAPTER *pAd, UCHAR OpMode, UCHAR ScanType,
							  struct wifi_dev *wdev,  UCHAR *desSsid, UCHAR desSsidLen)
{
	UCHAR backSsid[MAX_LEN_OF_SSID];
	UCHAR backSsidLen = 0;
	NdisZeroMemory(backSsid, MAX_LEN_OF_SSID);
	/* 1. backup the original MlmeAux */
	backSsidLen = pAd->ScanCtrl.SsidLen;
	NdisCopyMemory(backSsid, pAd->ScanCtrl.Ssid, backSsidLen);
	/* 2. fill the desried ssid into SM */
	pAd->ScanCtrl.SsidLen = desSsidLen;
	NdisCopyMemory(pAd->ScanCtrl.Ssid, desSsid, desSsidLen);
	/* 3. scan action */
	scan_active(pAd, OpMode, ScanType, wdev);
	/* 4. restore to ScanCtrl */
	pAd->ScanCtrl.SsidLen  = backSsidLen;
	NdisCopyMemory(pAd->ScanCtrl.Ssid, backSsid, backSsidLen);
}
#endif /* APCLI_SUPPORT */
/*
	==========================================================================
	Description:
		Scan next channel
	==========================================================================
 */
VOID ScanNextChannel(RTMP_ADAPTER *pAd, UCHAR OpMode, struct wifi_dev *pwdev)
{
	UCHAR ScanType = SCAN_TYPE_MAX;
	UINT ScanTimeIn5gChannel = SHORT_CHANNEL_TIME;
	BOOLEAN ScanPending = FALSE;
	RALINK_TIMER_STRUCT *sc_timer = NULL;
	UINT stay_time = 0;
	struct wifi_dev *wdev = pwdev;
	/* TODO: Star, fix me when Scan is prepare to modify */
	/* TODO: Star, fix me when Scan is prepare to modify */
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd) {
		if (wdev == NULL)
			wdev = &pAd->ApCfg.MBSSID[MAIN_MBSSID].wdev;
	}
#endif


#ifdef CONFIG_ATE

	/* Nothing to do in ATE mode. */
	if (ATE_ON(pAd))
		return;

#endif /* CONFIG_ATE */
#ifdef CONFIG_AP_SUPPORT

	if (OpMode == OPMODE_AP)
		ScanType = pAd->ScanCtrl.ScanType;

#endif /* CONFIG_AP_SUPPORT */

	if (ScanType == SCAN_TYPE_MAX) {
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s():Incorrect ScanType!\n", __func__));
		return;
	}


	if (TEST_FLAG_CONN_IN_PROG(pAd->ConInPrgress)) {
		/* Connection is in progress so no need to switch channel duing scan process */
		pAd->ScanCtrl.Channel = 0;
		MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
				("%s():Connection in prog, do not skip to other chan\n", __func__));
	}

	if ((pAd->ScanCtrl.Channel == 0) || ScanPending) {
#ifdef RT_CFG80211_SUPPORT
		//pAd->cfg80211_ctrl.Cfg80211CurChanIndex--;
#endif /* RT_CFG80211_SUPPORT */
		scan_ch_restore(pAd, OpMode, wdev);


	}

	else {
		{
			wlan_operate_scan(wdev, pAd->ScanCtrl.Channel);
		}
		{
			BOOLEAN bScanPassive = FALSE;

			if (pAd->ScanCtrl.Channel > 14) {

				if ((pAd->CommonCfg.bIEEE80211H == 1)
					&& RadarChannelCheck(pAd, pAd->ScanCtrl.Channel))
					bScanPassive = TRUE;

			}

#ifdef CARRIER_DETECTION_SUPPORT

			if (pAd->CommonCfg.CarrierDetect.Enable == TRUE)
				bScanPassive = TRUE;

#endif /* CARRIER_DETECTION_SUPPORT */

			if (bScanPassive) {
				ScanType = SCAN_PASSIVE;
				ScanTimeIn5gChannel = MIN_CHANNEL_TIME;
			}
		}

		/* Check if channel if passive scan under current regulatory domain */
		if (CHAN_PropertyCheck(pAd, pAd->ScanCtrl.Channel, CHANNEL_PASSIVE_SCAN) == TRUE)
			ScanType = SCAN_PASSIVE;

#if defined(DPA_T) || defined(WIFI_REGION32_HIDDEN_SSID_SUPPORT)

		/* Ch 12~14 is passive scan, No matter DFS and 80211H setting is y or n */
		if ((pAd->ScanCtrl.Channel >= 12) && (pAd->ScanCtrl.Channel <= 14))
			ScanType = SCAN_PASSIVE;

#endif /* DPA_T */
#ifdef CONFIG_AP_SUPPORT

		if (OpMode == OPMODE_AP)
			sc_timer = &pAd->ScanCtrl.APScanTimer;

#endif /* CONFIG_AP_SUPPORT */

		if (!sc_timer) {
			MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s():ScanTimer not assigned!\n", __func__));
			return;
		}

		/* We need to shorten active scan time in order for WZC connect issue */
		/* Chnage the channel scan time for CISCO stuff based on its IAPP announcement */
		if (ScanType == FAST_SCAN_ACTIVE)
			stay_time = FAST_ACTIVE_SCAN_TIME;
		else { /* must be SCAN_PASSIVE or SCAN_ACTIVE*/
#ifdef CONFIG_AP_SUPPORT

			if ((OpMode == OPMODE_AP) && (pAd->ApCfg.bAutoChannelAtBootup))
				stay_time = AUTO_CHANNEL_SEL_TIMEOUT;
			else
#endif /* CONFIG_AP_SUPPORT */
				if (WMODE_CAP_2G(wdev->PhyMode) &&
					WMODE_CAP_5G(wdev->PhyMode)) {
					if (pAd->ScanCtrl.Channel > 14)
						stay_time = ScanTimeIn5gChannel;
					else
						stay_time = MIN_CHANNEL_TIME;

				} else {
						stay_time = MAX_CHANNEL_TIME;
				}
		}

		RTMPSetTimer(sc_timer, stay_time);

		if (SCAN_MODE_ACT(ScanType)) {
#ifdef APCLI_SUPPORT
#endif
			if (scan_active(pAd, OpMode, ScanType, wdev) == FALSE){
				return;
			}

#ifdef APCLI_SUPPORT
#endif

			{
#ifdef APCLI_SUPPORT
				PAPCLI_STRUCT pApCliEntry = NULL;
				UINT index = 0;
				BOOLEAN needUnicastScan = FALSE;
#ifdef APCLI_AUTO_CONNECT_SUPPORT
				needUnicastScan = pAd->ApCfg.ApCliAutoConnectRunning[wdev->func_idx];
#endif /* APCLI_AUTO_CONNECT_SUPPORT */
#ifdef AP_PARTIAL_SCAN_SUPPORT
				needUnicastScan |= pAd->ApCfg.bPartialScanning;
#endif /* AP_PARTIAL_SCAN_SUPPORT */

				for (index = 0; index < MAX_APCLI_NUM; index++) {
					pApCliEntry = &pAd->ApCfg.ApCliTab[index];

					if (needUnicastScan && pApCliEntry->CfgSsidLen > 0) {
						FireExtraProbeReq(pAd,  OpMode, ScanType, wdev,
										  pApCliEntry->CfgSsid, pApCliEntry->CfgSsidLen);
					}
				}

#endif /* APCLI_SUPPORT */
			}

		}

		/* For SCAN_CISCO_PASSIVE, do nothing and silently wait for beacon or other probe reponse*/
#ifdef CONFIG_AP_SUPPORT

		if (OpMode == OPMODE_AP)
			pAd->Mlme.ApSyncMachine.CurrState = AP_SCAN_LISTEN;

#endif /* CONFIG_AP_SUPPORT */
	}
}


BOOLEAN ScanRunning(RTMP_ADAPTER *pAd)
{
	BOOLEAN	rv = FALSE;
#ifdef CONFIG_AP_SUPPORT
#ifdef AP_SCAN_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	rv = ((pAd->Mlme.ApSyncMachine.CurrState == AP_SCAN_LISTEN) ? TRUE : FALSE);
#endif /* AP_SCAN_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
	return rv;
}


/*
	==========================================================================
	Description:

	Return:
		scan_channel - channel to scan.
	Note:
		return 0 if no more next channel
	==========================================================================
 */
UCHAR FindScanChannel(
	RTMP_ADAPTER *pAd,
	UINT8 LastScanChannel,
	struct wifi_dev *wdev)
{
	UCHAR scan_channel = 0;

	if (pAd->ScanCtrl.PartialScan.bScanning == TRUE) {
		scan_channel = FindPartialScanChannel(pAd, wdev);
		return scan_channel;
	}

	if (LastScanChannel == 0)
		scan_channel = FirstChannel(pAd, wdev);
	else
		scan_channel = NextChannel(pAd, LastScanChannel, wdev);

	return scan_channel;
}


/*
	==========================================================================
	Description:

	Return:
		scan_channel - channel to scan.
	Note:
		return 0 if no more next channel
	==========================================================================
 */
UCHAR FindPartialScanChannel(RTMP_ADAPTER *pAd, struct wifi_dev *wdev)
{
	UCHAR scan_channel = 0;
	PARTIAL_SCAN *PartialScanCtrl = &pAd->ScanCtrl.PartialScan;

	if (PartialScanCtrl->NumOfChannels > 0) {
		PartialScanCtrl->NumOfChannels--;

		if (PartialScanCtrl->LastScanChannel == 0)
			scan_channel = FirstChannel(pAd, wdev);
		else
			scan_channel = NextChannel(pAd, PartialScanCtrl->LastScanChannel, wdev);

		/* update last scanned channel */
		PartialScanCtrl->LastScanChannel = scan_channel;

		if (scan_channel == 0) {
			PartialScanCtrl->BreakTime = 0;
			PartialScanCtrl->bScanning = FALSE;
			PartialScanCtrl->pwdev = NULL;
			PartialScanCtrl->NumOfChannels = DEFLAUT_PARTIAL_SCAN_CH_NUM;
		}
	} else {
		/* Pending for next partial scan */
		scan_channel = 0;
		PartialScanCtrl->NumOfChannels = DEFLAUT_PARTIAL_SCAN_CH_NUM;
	}

	{
	MTWF_LOG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_TRACE,
			 ("%s, %u, scan_channel = %u, NumOfChannels = %u, LastScanChannel = %u, bScanning = %u\n",
			  __func__, __LINE__,
			  scan_channel,
			  PartialScanCtrl->NumOfChannels,
			  PartialScanCtrl->LastScanChannel,
			  PartialScanCtrl->bScanning));
	}
	return scan_channel;
}


INT PartialScanInit(RTMP_ADAPTER *pAd)
{
	PARTIAL_SCAN *PartialScanCtrl = &pAd->ScanCtrl.PartialScan;
	PartialScanCtrl->bScanning = FALSE;
	PartialScanCtrl->NumOfChannels = DEFLAUT_PARTIAL_SCAN_CH_NUM;
	PartialScanCtrl->LastScanChannel = 0;
	PartialScanCtrl->BreakTime = 0;

	return 0;
}

#if defined(CONFIG_STA_SUPPORT) || defined(APCLI_SUPPORT)
/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL

	==========================================================================
*/
VOID ScanParmFill(
	IN RTMP_ADAPTER *pAd,
	IN OUT MLME_SCAN_REQ_STRUCT *ScanReq,
	IN RTMP_STRING Ssid[],
	IN UCHAR SsidLen,
	IN UCHAR BssType,
	IN UCHAR ScanType)
{
	NdisZeroMemory(ScanReq->Ssid, MAX_LEN_OF_SSID);
	ScanReq->SsidLen = (SsidLen > MAX_LEN_OF_SSID) ? MAX_LEN_OF_SSID : SsidLen;
	NdisMoveMemory(ScanReq->Ssid, Ssid, ScanReq->SsidLen);
	ScanReq->BssType = BssType;
	ScanReq->ScanType = ScanType;
}
#endif /* defined(CONFIG_STA_SUPPORT) || defined(APCLI_SUPPORT) */
#endif /* SCAN_SUPPORT */


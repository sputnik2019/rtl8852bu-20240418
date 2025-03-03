
/******************************************************************************
 *
 * Copyright(c) 2020 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
#define _PHL_DM_C_
#include "phl_headers.h"

void rtw_phl_set_edcca_mode(void *phl, enum rtw_edcca_mode mode)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;

	PHL_INFO("[Cert], set phl_com edcca_mode : %d !! \n", mode);
	phl_info->phl_com->edcca_mode = mode;

	rtw_phl_cmd_edcca_mode_cfg(phl, mode, PHL_CMD_NO_WAIT, 0);
}

enum rtw_edcca_mode rtw_phl_get_edcca_mode(void *phl)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;

	return phl_info->phl_com->edcca_mode;
}

enum rtw_phl_status
phl_edcca_cfg(struct phl_info_t *phl_info)
{
	struct rtw_phl_com_t *phl_com = phl_info->phl_com;
	enum rtw_phl_status phl_status = RTW_PHL_STATUS_SUCCESS;
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_SUCCESS;

	if (RTW_EDCCA_FCC == phl_com->edcca_mode) {
		hal_status = rtw_hal_sifs_chk_cca_en(phl_info->hal, HW_BAND_0, true);
		if (hal_status != RTW_HAL_STATUS_SUCCESS) {
			PHL_ERR("%s: enable rtw_hal_sifs_chk_cca_en failed!\n", __FUNCTION__);
			phl_status = RTW_PHL_STATUS_FAILURE;
		}

		hal_status = rtw_hal_set_resp_ack_chk_cca(phl_info->hal, HW_BAND_0, true);
		if (hal_status != RTW_HAL_STATUS_SUCCESS) {
			PHL_ERR("%s: enable rtw_hal_set_resp_ack_chk_cca failed!\n", __FUNCTION__);
			phl_status = RTW_PHL_STATUS_FAILURE;
		}
	} else {
		hal_status = rtw_hal_sifs_chk_cca_en(phl_info->hal, HW_BAND_0, false);
		if (hal_status != RTW_HAL_STATUS_SUCCESS) {
			PHL_ERR("%s: disable rtw_hal_sifs_chk_cca_en failed!\n", __FUNCTION__);
			phl_status = RTW_PHL_STATUS_FAILURE;
		}

		hal_status = rtw_hal_set_resp_ack_chk_cca(phl_info->hal, HW_BAND_0, false);
		if (hal_status != RTW_HAL_STATUS_SUCCESS) {
			PHL_ERR("%s: disable rtw_hal_set_resp_ack_chk_cca failed!\n", __FUNCTION__);
			phl_status = RTW_PHL_STATUS_FAILURE;
		}
	}

	return phl_status;
}

enum rtw_phl_status
phl_cmd_edcca_cfg_hdl(struct phl_info_t *phl_info, u8 *param)
{
	return phl_edcca_cfg(phl_info);
}

enum rtw_phl_status
rtw_phl_cmd_edcca_mode_cfg(void *phl,
			   enum rtw_edcca_mode mode,
                           enum phl_cmd_type cmd_type,
                           u32 cmd_timeout)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;
	enum rtw_phl_status psts = RTW_PHL_STATUS_FAILURE;

	phl_info->phl_com->edcca_mode = mode;

#ifdef CONFIG_CMD_DISP
	if (cmd_type == PHL_CMD_DIRECTLY) {
		psts = phl_edcca_cfg(phl_info);
		goto _exit;
	}

	psts = phl_cmd_enqueue(phl_info,
			HW_BAND_0,
			MSG_EVT_EDCCA_CFG,
			NULL, 0,
			NULL,
			cmd_type, cmd_timeout);

	if (is_cmd_failure(psts)) {
		/* Send cmd success, but wait cmd fail*/
		psts = RTW_PHL_STATUS_FAILURE;
	} else if (psts != RTW_PHL_STATUS_SUCCESS) {
		/* Send cmd fail */
		psts = RTW_PHL_STATUS_FAILURE;
	}

_exit:
	return psts;
#else
	PHL_ERR("%s : CONFIG_CMD_DISP need to be enabled for MSG_EVT_EDCCA_CFG !! \n", __func__);
	return psts;
#endif
}

#ifdef CONFIG_PCI_HCI
#ifdef RTW_WKARD_DYNAMIC_LTR
enum rtw_phl_status
phl_ltr_sw_trigger(struct rtw_phl_com_t *phl_com, void *hal,
	enum rtw_pcie_ltr_state state)
{
	enum rtw_hal_status status = RTW_HAL_STATUS_FAILURE;
	struct rtw_stats *phl_stats = &phl_com->phl_stats;

	status = rtw_hal_ltr_sw_trigger(hal, state);

	if (status == RTW_HAL_STATUS_SUCCESS) {
		phl_stats->ltr_cur_state = state;
		phl_stats->ltr_last_trigger_time = _os_get_cur_time_us();
		state == RTW_PCIE_LTR_SW_ACT ?
		phl_stats->ltr_sw_act_tri_cnt++ : phl_stats->ltr_sw_idle_tri_cnt++;
		return RTW_PHL_STATUS_SUCCESS;
	} else {
		return RTW_PHL_STATUS_FAILURE;
	}
}

/*  Switching sw ctrl will trigger active ltr at the same time
to prevent inconsistent state */
/* usage : echo phl ltr set [enable/disable] */
enum rtw_phl_status
phl_ltr_sw_ctrl(struct rtw_phl_com_t *phl_com, void *hal, bool enable)
{
	enum rtw_hal_status status = RTW_HAL_STATUS_FAILURE;

	status = phl_ltr_sw_trigger(phl_com, hal, RTW_PCIE_LTR_SW_ACT);

	if (status == RTW_HAL_STATUS_SUCCESS) {
		phl_com->bus_sw_cap.ltr_sw_ctrl = enable;
		return RTW_PHL_STATUS_SUCCESS;
	} else {
		return RTW_PHL_STATUS_FAILURE;
	}
}

/* switch to hw control. it's valid that only hw supports hw mode */
/* usage : echo phl ltr set [enable/disable] */
void
phl_ltr_hw_ctrl(struct rtw_phl_com_t *phl_com, bool enable)
{
	phl_com->bus_sw_cap.ltr_hw_ctrl = enable;
}

/* For pm module, this will not trigger active ltr since halmac will take care of*/
void phl_ltr_sw_ctrl_ntfy(struct rtw_phl_com_t *phl_com, bool enable)
{
	phl_com->bus_sw_cap.ltr_sw_ctrl = enable;
}

u8 phl_ltr_get_cur_state(struct rtw_phl_com_t *phl_com)
{
	return phl_com->phl_stats.ltr_cur_state;
}

u32 phl_ltr_get_last_trigger_time(struct rtw_phl_com_t *phl_com)
{
	return phl_com->phl_stats.ltr_last_trigger_time;
}

u32 phl_ltr_get_tri_cnt(struct rtw_phl_com_t *phl_com,
	enum rtw_pcie_ltr_state state)
{
	struct rtw_stats *phl_stats = &phl_com->phl_stats;

	return state == RTW_PCIE_LTR_SW_ACT ?
			phl_stats->ltr_sw_act_tri_cnt : phl_stats->ltr_sw_idle_tri_cnt;
}

#define TP_MBPS 100
void phl_ltr_ctrl_watchdog(struct phl_info_t *phl_info)
{
	struct rtw_phl_com_t *phl_com  = phl_info->phl_com;
	struct rtw_stats *sts = &phl_com->phl_stats;
	u32 tx_tp_m = 0, rx_tp_m = 0;
	static bool start = false;

	/* only valid if it's currenlty running hw mode */
	if (!rtw_hal_ltr_is_hw_ctrl(phl_com, phl_info->hal))
		return;

	tx_tp_m = sts->tx_tp_kbits >> 10;
	rx_tp_m = sts->rx_tp_kbits >> 10;

	/* PHL_INFO("%s tx_tp_m = %u /rx_tp_m = %u \n", __func__, tx_tp_m, rx_tp_m);*/

	if ((tx_tp_m > TP_MBPS || rx_tp_m > TP_MBPS) && !start) {
		start = true;
		rtw_hal_ltr_en_hw_mode(phl_info->hal, false);
		rtw_hal_ltr_sw_trigger(phl_info->hal, RTW_PCIE_LTR_SW_ACT);
	}

	if (start && tx_tp_m < TP_MBPS && rx_tp_m < TP_MBPS) {
		start = false;
		rtw_hal_ltr_sw_trigger(phl_info->hal, RTW_PCIE_LTR_SW_IDLE);
		rtw_hal_ltr_en_hw_mode(phl_info->hal, true);
	}
}
#endif /* RTW_WKARD_DYNAMIC_LTR */
#endif /* CONFIG_PCI_HCI */
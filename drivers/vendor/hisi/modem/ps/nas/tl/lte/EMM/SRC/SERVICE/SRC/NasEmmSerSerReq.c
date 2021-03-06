


/*****************************************************************************
  1 Include HeadFile
*****************************************************************************/
#include "NasEmmTauSerInclude.h"
#include "EmmTcInterface.h"
#include "NasEmmAttDetInclude.h"

#include "ImsaIntraInterface.h"
#if (FEATURE_LPP == FEATURE_ON)
#include "NasEmmLppMsgProc.h"
#include "EmmLppInterface.h"
#endif
#include "NasLmmPubMMsgBuf.h"
/*lint -e767*/
#define    THIS_FILE_ID            PS_FILE_ID_NASEMMSERVICESERREQ_C
#define    THIS_NAS_FILE_ID        NAS_FILE_ID_NASEMMSERVICESERREQ_C
/*lint +e767*/



/*****************************************************************************
  1.1 Cplusplus Announce
*****************************************************************************/
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

VOS_UINT8  g_ucConnStateRcvDrbReestTimes   = 0;


VOS_UINT32 NAS_EMM_MsRegSsLimitedSrvMsgRabmReestReq
(
    VOS_UINT32                          ulMsgId,
    VOS_VOID                           *pMsgStru
)
{
    EMM_ERABM_REEST_REQ_STRU           *pstReestReq = NAS_EMM_NULL_PTR;

    (VOS_VOID)ulMsgId;

    NAS_EMM_SER_LOG_INFO( "NAS_EMM_MsRegSsLimitedSrvMsgRabmReestReq is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegSsLimitedSrvMsgRabmReestReq_ENUM,LNAS_ENTRY);

    pstReestReq = (EMM_ERABM_REEST_REQ_STRU *)pMsgStru;

    /* 如果不是紧急业务，则丢弃 */
    if (VOS_TRUE != pstReestReq->ulIsEmcType)
    {
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_MsRegSsLimitedSrvMsgRabmReestReq:Not EMC!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsLimitedSrvMsgRabmReestReq_ENUM,LNAS_FUNCTION_LABEL1);
        return NAS_LMM_MSG_DISCARD;
    }

    /*如果处于连接态，打印出错信息*/
    if (NAS_EMM_YES == NAS_EMM_IsNotIdle())
    {
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_MsRegSsLimitedSrvMsgRabmReestReq: CONN.");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsLimitedSrvMsgRabmReestReq_ENUM,LNAS_FUNCTION_LABEL1);

        /* T3440定时器运行中时丢弃该消息, 解决当TAU成功后启动T3440定时器(此时LNAS和LRRC都处于数据连接态)
           时, 连续收到ERABM的DRB重建消息, 导致LMM强制把连接状态转为IDLE态, 而此时LRRC为连接态, 从而导致
           后续流程异常, 出现复位 */
        if (NAS_LMM_TIMER_RUNNING == NAS_LMM_IsStaTimerRunning(TI_NAS_EMM_STATE_T3440))
        {
            NAS_EMM_SER_LOG_WARN( "NAS_EMM_MsRegSsNormalMsgRabmReestReq T3440 is Running !!");
            TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsNormalMsgRabmReestReq_ENUM,LNAS_FUNCTION_LABEL3);
            return NAS_LMM_MSG_HANDLED;
        }

        /* 使用全局变量进行计数, 当后面发起Service时、收到RRC的释放或者DRB建立成功时清除, 解决:带Active
           标志的TAU成功后, 在网侧建DRB期间, CDS有上行数据要发, ERABM发送DRB_REEST_REQ,
           由于是使用静态局部变量进行计数, 只能在该函数清变量, 从而导致三次进入该函数时
           强制把连接状态转为IDLE, 导致后续流程异常 */
        g_ucConnStateRcvDrbReestTimes++;

        /* 做保护,防止EMM与RABM维护的RRC链路状态不一致,导致UE长时间无法发起建链*/
        /* 连续10次收到ERABM的建链请求时,将RRC链路状态设置为IDLE态*/
        if (NAS_EMM_DISCARD_ERABM_RESET_REQ_MAX_CNT > g_ucConnStateRcvDrbReestTimes)
        {
            return NAS_LMM_MSG_HANDLED;
        }

        /* 通知LRRC释放 */
        NAS_EMM_RelReq(NAS_LMM_NOT_BARRED);
    }

    g_ucConnStateRcvDrbReestTimes = NAS_EMM_NULL;

    NAS_EMM_SER_RcvRabmReestReq(pstReestReq->ulIsEmcType);

    return NAS_LMM_MSG_HANDLED;
}

VOS_UINT32 NAS_EMM_MsRegSsNormalMsgRabmReestReq(VOS_UINT32  ulMsgId,
                                                   VOS_VOID   *pMsgStru
                               )
{
    VOS_UINT32                          ulRslt      = NAS_EMM_FAIL;
    EMM_ERABM_REEST_REQ_STRU           *pstReestReq = NAS_EMM_NULL_PTR;

    (VOS_VOID)ulMsgId;

    NAS_EMM_SER_LOG_INFO( "Nas_Emm_MsRegSsNormalMsgRabmReestReq is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegSsNormalMsgRabmReestReq_ENUM,LNAS_ENTRY);

    /*检查当前状态和输入指针*/
    ulRslt = NAS_EMM_SER_CHKFSMStateMsgp(EMM_MS_REG,EMM_SS_REG_NORMAL_SERVICE,pMsgStru);
    if ( NAS_EMM_SUCC != ulRslt )
    {
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_TAUSER_CHKFSMStateMsgp ERROR !!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsNormalMsgRabmReestReq_ENUM,LNAS_ERROR);
         return NAS_LMM_MSG_HANDLED;
    }

    /*如果处于连接态，打印出错信息*/
    if (NAS_EMM_YES == NAS_EMM_IsNotIdle())
    {
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_MsRegSsNormalMsgRabmReestReq: CONN.");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsNormalMsgRabmReestReq_ENUM,LNAS_FUNCTION_LABEL1);

        /* T3440定时器运行中时丢弃该消息, 解决当TAU成功后启动T3440定时器(此时LNAS和LRRC都处于数据连接态)
           时, 连续收到ERABM的DRB重建消息, 导致LMM强制把连接状态转为IDLE态, 而此时LRRC为连接态, 从而导致
           后续流程异常, 出现复位 */
        if (NAS_LMM_TIMER_RUNNING == NAS_LMM_IsStaTimerRunning(TI_NAS_EMM_STATE_T3440))
        {
            NAS_EMM_SER_LOG_WARN( "NAS_EMM_MsRegSsNormalMsgRabmReestReq T3440 is Running !!");
            TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsNormalMsgRabmReestReq_ENUM,LNAS_FUNCTION_LABEL3);
            return NAS_LMM_MSG_HANDLED;
        }

        /* 使用全局变量进行计数, 当后面发起Service时、收到RRC的释放或者DRB建立成功时清除, 解决:带Active
           标志的TAU成功后, 在网侧建DRB期间, CDS有上行数据要发, ERABM发送DRB_REEST_REQ,
           由于是使用静态局部变量进行计数, 只能在该函数清变量, 从而导致三次进入该函数时
           强制把连接状态转为IDLE, 导致后续流程异常 */
        g_ucConnStateRcvDrbReestTimes++;

        /* 做保护,防止EMM与RABM维护的RRC链路状态不一致,导致UE长时间无法发起建链*/
        /* 连续10次收到ERABM的建链请求时,将RRC链路状态设置为IDLE态*/
        if (NAS_EMM_DISCARD_ERABM_RESET_REQ_MAX_CNT > g_ucConnStateRcvDrbReestTimes)
        {
            return NAS_LMM_MSG_HANDLED;
        }

        /* 通知LRRC释放 */
        NAS_EMM_RelReq(NAS_LMM_NOT_BARRED);
    }
    g_ucConnStateRcvDrbReestTimes = NAS_EMM_NULL;

    if((NAS_LMM_TIMER_RUNNING == NAS_LMM_IsPtlTimerRunning(TI_NAS_EMM_PTL_CSFB_DELAY))
        &&(VOS_TRUE == NAS_EMM_SER_IsCsfbProcedure()))
    {
        NAS_EMM_SER_LOG_INFO( "NAS_EMM_MsRegSsNormalMsgRabmReestReq: Msg discard, CSFB delay timer is running.");
        TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegSsNormalMsgRabmReestReq_ENUM,LNAS_FUNCTION_LABEL2);
        return NAS_LMM_MSG_HANDLED;
    }

    pstReestReq = (EMM_ERABM_REEST_REQ_STRU *)pMsgStru;
    NAS_EMM_SER_RcvRabmReestReq(pstReestReq->ulIsEmcType);
    return NAS_LMM_MSG_HANDLED;

}


VOS_UINT32  NAS_EMM_MsRegSsRegAttemptUpdateMmMsgRabmReestReq
(
    VOS_UINT32                          ulMsgId,
    VOS_VOID                           *pMsgStru
)
{
    VOS_UINT32                          ulRslt      = NAS_EMM_FAIL;
    EMM_ERABM_REEST_REQ_STRU           *pstReestReq = NAS_EMM_NULL_PTR;

    (VOS_VOID)ulMsgId;

    NAS_EMM_SER_LOG_INFO( "NAS_EMM_MsRegSsRegAttemptUpdateMmMsgRabmReestReq is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegSsRegAttemptUpdateMmMsgRabmReestReq_ENUM,LNAS_ENTRY);

    /*检查当前状态和输入指针*/
    ulRslt = NAS_EMM_SER_CHKFSMStateMsgp(EMM_MS_REG,
                                        EMM_SS_REG_ATTEMPTING_TO_UPDATE_MM,
                                        pMsgStru);
    if ( NAS_EMM_SUCC != ulRslt )
    {
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_TAUSER_CHKFSMStateMsgp ERROR !!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsRegAttemptUpdateMmMsgRabmReestReq_ENUM,LNAS_ERROR);
         return NAS_LMM_MSG_HANDLED;
    }

    /*如果处于连接态，打印出错信息*/
    if (NAS_EMM_YES == NAS_EMM_IsNotIdle())
    {
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_MsRegSsRegAttemptUpdateMmMsgRabmReestReq: CONN.");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsRegAttemptUpdateMmMsgRabmReestReq_ENUM,LNAS_FUNCTION_LABEL1);

        /* T3440定时器运行中时丢弃该消息, 解决当TAU成功后启动T3440定时器(此时LNAS和LRRC都处于数据连接态)
           时, 连续收到ERABM的DRB重建消息, 导致LMM强制把连接状态转为IDLE态, 而此时LRRC为连接态, 从而导致
           后续流程异常, 出现复位 */
        if (NAS_LMM_TIMER_RUNNING == NAS_LMM_IsStaTimerRunning(TI_NAS_EMM_STATE_T3440))
        {
            NAS_EMM_SER_LOG_WARN( "NAS_EMM_MsRegSsNormalMsgRabmReestReq T3440 is Running !!");
            TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsNormalMsgRabmReestReq_ENUM,LNAS_FUNCTION_LABEL3);
            return NAS_LMM_MSG_HANDLED;
        }

        /* 使用全局变量进行计数, 当后面发起Service时、收到RRC的释放或者DRB建立成功时清除, 解决:带Active
           标志的TAU成功后, 在网侧建DRB期间, CDS有上行数据要发, ERABM发送DRB_REEST_REQ,
           由于是使用静态局部变量进行计数, 只能在该函数清变量, 从而导致三次进入该函数时
           强制把连接状态转为IDLE, 导致后续流程异常 */
        g_ucConnStateRcvDrbReestTimes++;

        /* 做保护,防止EMM与RABM维护的RRC链路状态不一致,导致UE长时间无法发起建链*/
        /* 连续2次收到ERABM的建链请求时,将RRC链路状态设置为IDLE态*/
        if (NAS_EMM_DISCARD_ERABM_RESET_REQ_MAX_CNT > g_ucConnStateRcvDrbReestTimes)
        {
            return NAS_LMM_MSG_HANDLED;
        }

        /* 通知LRRC释放 */
        NAS_EMM_RelReq(NAS_LMM_NOT_BARRED);
    }

    g_ucConnStateRcvDrbReestTimes = NAS_EMM_NULL;

    /* 记录UPDATE_MM标识 */
    /*NAS_LMM_SetEmmInfoUpdateMmFlag(NAS_EMM_UPDATE_MM_FLAG_VALID);*/
    pstReestReq = (EMM_ERABM_REEST_REQ_STRU *)pMsgStru;
    NAS_EMM_SER_RcvRabmReestReq(pstReestReq->ulIsEmcType);

    return NAS_LMM_MSG_HANDLED;
}


VOS_UINT32  NAS_EMM_MsRegSsLimitedSrvMsgRabmRelReq
(
    VOS_UINT32                          ulMsgId,
    VOS_VOID                           *pMsgStru
)
{
    NAS_EMM_SER_LOG_INFO("NAS_EMM_MsRegSsLimitedSrvMsgRabmRelReq entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegSsLimitedSrvMsgRabmRelReq_ENUM,LNAS_ENTRY);
    (VOS_VOID)ulMsgId;
    (VOS_VOID)pMsgStru;

    /*如果是释放过程中，则直接丢弃*/
    if(NAS_EMM_CONN_RELEASING == NAS_EMM_GetConnState())
    {
        return NAS_LMM_MSG_HANDLED;
    }

    if(NAS_LMM_TIMER_RUNNING == NAS_LMM_IsStaTimerRunning(TI_NAS_EMM_STATE_T3440))
    {
        NAS_LMM_StopStateTimer(TI_NAS_EMM_STATE_T3440);

        /*启动定时器TI_NAS_EMM_MRRC_WAIT_RRC_REL*/
        NAS_LMM_StartStateTimer(TI_NAS_EMM_MRRC_WAIT_RRC_REL_CNF);

        /*向MRRC发送NAS_EMM_MRRC_REL_REQ消息*/
        NAS_EMM_SndRrcRelReq(NAS_LMM_NOT_BARRED);

        /* 设置连接状态为释放过程中 */
        NAS_EMM_SetConnState(NAS_EMM_CONN_RELEASING);

        return NAS_LMM_MSG_HANDLED;
    }


    /*向MRRC发送NAS_EMM_MRRC_REL_REQ消息*/
    NAS_EMM_RelReq(                     NAS_LMM_NOT_BARRED);

    return NAS_LMM_MSG_HANDLED;
}

/*****************************************************************************
 Function Name   : NAS_EMM_MsRegSsNormalMsgRabmRelReq
 Description     : Reg.Normal_Service状态下收到RABM数传异常，释放连接
 Input           : None
 Output          : None
 Return          : VOS_UINT32

 History         :
    1.sunbing      2010-12-29  Draft Enact

*****************************************************************************/
VOS_UINT32  NAS_EMM_MsRegSsNormalMsgRabmRelReq( VOS_UINT32  ulMsgId,
                                                   VOS_VOID   *pMsgStru )
{
    NAS_EMM_SER_LOG_INFO("NAS_EMM_MsRegSsNormalMsgRabmRelReq entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegSsNormalMsgRabmRelReq_ENUM,LNAS_ENTRY);
    (VOS_VOID)ulMsgId;
    (VOS_VOID)pMsgStru;

    /*如果是释放过程中，则直接丢弃*/
    if(NAS_EMM_CONN_RELEASING == NAS_EMM_GetConnState())
    {
        return NAS_LMM_MSG_HANDLED;
    }

    if(NAS_LMM_TIMER_RUNNING == NAS_LMM_IsStaTimerRunning(TI_NAS_EMM_STATE_T3440))
    {
        NAS_LMM_StopStateTimer(TI_NAS_EMM_STATE_T3440);

        /*启动定时器TI_NAS_EMM_MRRC_WAIT_RRC_REL*/
        NAS_LMM_StartStateTimer(TI_NAS_EMM_MRRC_WAIT_RRC_REL_CNF);

        /*向MRRC发送NAS_EMM_MRRC_REL_REQ消息*/
        NAS_EMM_SndRrcRelReq(NAS_LMM_NOT_BARRED);

        /* 设置连接状态为释放过程中 */
        NAS_EMM_SetConnState(NAS_EMM_CONN_RELEASING);

        return NAS_LMM_MSG_HANDLED;
    }

    /*向MRRC发送NAS_EMM_MRRC_REL_REQ消息*/
    NAS_EMM_RelReq(                     NAS_LMM_NOT_BARRED);

    return NAS_LMM_MSG_HANDLED;

}


VOS_UINT32  NAS_EMM_MsRegSsRegAttemptUpdateMmMsgRabmRelReq
(
    VOS_UINT32                          ulMsgId,
    VOS_VOID                           *pMsgStru
)
{
    NAS_EMM_SER_LOG_INFO("NAS_EMM_MsRegSsRegAttemptUpdateMmMsgRabmRelReq entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegSsRegAttemptUpdateMmMsgRabmRelReq_ENUM,LNAS_ENTRY);

    (VOS_VOID)ulMsgId;
    (VOS_VOID)pMsgStru;

    /*如果是释放过程中，则直接丢弃*/
    if(NAS_EMM_CONN_RELEASING == NAS_EMM_GetConnState())
    {
        return NAS_LMM_MSG_HANDLED;
    }

    if(NAS_LMM_TIMER_RUNNING == NAS_LMM_IsStaTimerRunning(TI_NAS_EMM_STATE_T3440))
    {
        NAS_LMM_StopStateTimer(TI_NAS_EMM_STATE_T3440);

        /*启动定时器TI_NAS_EMM_MRRC_WAIT_RRC_REL*/
        NAS_LMM_StartStateTimer(TI_NAS_EMM_MRRC_WAIT_RRC_REL_CNF);

        /*向MRRC发送NAS_EMM_MRRC_REL_REQ消息*/
        NAS_EMM_SndRrcRelReq(NAS_LMM_NOT_BARRED);

        /* 设置连接状态为释放过程中 */
        NAS_EMM_SetConnState(NAS_EMM_CONN_RELEASING);

        return NAS_LMM_MSG_HANDLED;
    }

    /*向MRRC发送NAS_EMM_MRRC_REL_REQ消息*/
    NAS_EMM_RelReq(                     NAS_LMM_NOT_BARRED);

    return NAS_LMM_MSG_HANDLED;
}


VOS_UINT32  NAS_EMM_MsRegInitSsWaitCnAttachCnfMsgTcDataReq
(
    VOS_UINT32                          ulMsgId,
    VOS_VOID                           *pMsgStru
)
{
    VOS_UINT32                          ulRslt;

    (VOS_VOID)ulMsgId;

    NAS_EMM_SER_LOG_INFO("NAS_EMM_MsRegInitSsWaitCnAttachCnfMsgTcDataReq is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegInitSsWaitCnAttachCnfMsgTcDataReq_ENUM,LNAS_ENTRY);

    ulRslt = NAS_EMM_SER_CHKFSMStateMsgp(EMM_MS_REG_INIT,EMM_SS_ATTACH_WAIT_CN_ATTACH_CNF,pMsgStru);
    if ( NAS_EMM_SUCC != ulRslt )
    {
        /* 打印异常 */
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_MsRegInitSsWaitCnAttachCnfMsgTcDataReq ERROR !!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegInitSsWaitCnAttachCnfMsgTcDataReq_ENUM,LNAS_ERROR);
        return NAS_LMM_MSG_DISCARD;
    }

    NAS_EMM_SER_SendMrrcDataReq_Tcdata((EMM_ETC_DATA_REQ_STRU *)pMsgStru);

    return NAS_LMM_MSG_HANDLED;
}



VOS_UINT32  NAS_EMM_MsRegSsNormalMsgTcDataReq
(
    VOS_UINT32                          ulMsgId,
    VOS_VOID                           *pMsgStru
)
{
    VOS_UINT32                      ulRslt              = NAS_EMM_FAIL;

    (VOS_VOID)ulMsgId;

    NAS_EMM_SER_LOG_INFO("NAS_EMM_MsRegSsNormalMsgTcDataReq is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegSsNormalMsgTcDataReq_ENUM,LNAS_ENTRY);

    /* 函数输入指针参数检查, 状态匹配检查,若不匹配,退出*/
    ulRslt = NAS_EMM_SER_CHKFSMStateMsgp(EMM_MS_REG,EMM_SS_REG_NORMAL_SERVICE,pMsgStru);
    if ( NAS_EMM_SUCC != ulRslt )
    {
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_TAUSER_CHKFSMStateMsgp ERROR !!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsNormalMsgTcDataReq_ENUM,LNAS_ERROR);
        return NAS_LMM_MSG_DISCARD;
    }

    /*如果是释放过程中，则直接丢弃*/
    if(NAS_EMM_CONN_RELEASING == NAS_EMM_GetConnState())
    {
        return NAS_LMM_MSG_HANDLED;
    }

    if(NAS_LMM_TIMER_RUNNING == NAS_LMM_IsStaTimerRunning(TI_NAS_EMM_STATE_T3440))
    {
        /* 透传ETC消息 */
         NAS_EMM_SER_SendMrrcDataReq_Tcdata((EMM_ETC_DATA_REQ_STRU *)pMsgStru);
        return NAS_LMM_MSG_HANDLED;
    }


    /*CONN模式下，转发TC消息；IDLE模式下，打印出错信息*/
    if((NAS_EMM_CONN_SIG == NAS_EMM_GetConnState()) ||
        (NAS_EMM_CONN_DATA == NAS_EMM_GetConnState()))
    {
        NAS_EMM_SER_SendMrrcDataReq_Tcdata((EMM_ETC_DATA_REQ_STRU *)pMsgStru);
    }
    else
    {
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_MsRegSsNormalMsgTcDataReq:Warning:RRC connection is not Exist!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsNormalMsgTcDataReq_ENUM,LNAS_FUNCTION_LABEL1);
    }

    return NAS_LMM_MSG_HANDLED;

}


VOS_UINT32  NAS_EMM_MsRegSsRegAttemptUpdateMmMsgTcDataReq
(
    VOS_UINT32                          ulMsgId,
    VOS_VOID                           *pMsgStru
)
{
    VOS_UINT32                      ulRslt              = NAS_EMM_FAIL;

    (VOS_VOID)ulMsgId;

    NAS_EMM_SER_LOG_INFO("NAS_EMM_MsRegSsRegAttemptUpdateMmMsgTcDataReq is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegSsRegAttemptUpdateMmMsgTcDataReq_ENUM,LNAS_ENTRY);

    /* 函数输入指针参数检查, 状态匹配检查,若不匹配,退出*/
    ulRslt = NAS_EMM_SER_CHKFSMStateMsgp(EMM_MS_REG,
                                        EMM_SS_REG_ATTEMPTING_TO_UPDATE_MM,
                                        pMsgStru);
    if ( NAS_EMM_SUCC != ulRslt )
    {
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_TAUSER_CHKFSMStateMsgp ERROR !!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsRegAttemptUpdateMmMsgTcDataReq_ENUM,LNAS_ERROR);
        return NAS_LMM_MSG_DISCARD;
    }

    /*如果是释放过程中，则直接丢弃*/
    if(NAS_EMM_CONN_RELEASING == NAS_EMM_GetConnState())
    {
        return NAS_LMM_MSG_HANDLED;
    }

    if(NAS_LMM_TIMER_RUNNING == NAS_LMM_IsStaTimerRunning(TI_NAS_EMM_STATE_T3440))
    {
        /* 透传ETC消息 */
         NAS_EMM_SER_SendMrrcDataReq_Tcdata((EMM_ETC_DATA_REQ_STRU *)pMsgStru);
        return NAS_LMM_MSG_HANDLED;
    }


    /*CONN模式下，转发TC消息；IDLE模式下，打印出错信息*/
    if((NAS_EMM_CONN_SIG == NAS_EMM_GetConnState()) ||
        (NAS_EMM_CONN_DATA == NAS_EMM_GetConnState()))
    {
        NAS_EMM_SER_SendMrrcDataReq_Tcdata((EMM_ETC_DATA_REQ_STRU *)pMsgStru);
    }
    else
    {
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_MsRegSsRegAttemptUpdateMmMsgTcDataReq:Warning:RRC connection is not Exist!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsRegAttemptUpdateMmMsgTcDataReq_ENUM,LNAS_ERROR);
    }

    return NAS_LMM_MSG_HANDLED;
}



VOS_UINT32 NAS_EMM_MsRegInitSsWaitCnAttachCnfMsgEsmDataReq
(
    VOS_UINT32                          ulMsgId,
    VOS_VOID                           *pMsgStru
)
{
    VOS_UINT32                          ulRslt;
    EMM_ESM_DATA_REQ_STRU              *pstEsmDataReq = (EMM_ESM_DATA_REQ_STRU*)pMsgStru;

    (VOS_VOID)ulMsgId;

    NAS_EMM_SER_LOG_INFO("NAS_EMM_MsRegInitSsWaitCnAttachCnfMsgEsmDataReq is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegInitSsWaitCnAttachCnfMsgEsmDataReq_ENUM,LNAS_ENTRY);

    ulRslt = NAS_EMM_SER_CHKFSMStateMsgp(EMM_MS_REG_INIT,EMM_SS_ATTACH_WAIT_CN_ATTACH_CNF,pMsgStru);
    if ( NAS_EMM_SUCC != ulRslt )
    {
        /* 打印异常 */
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_MsRegInitSsWaitCnAttachCnfMsgEsmDataReq ERROR !!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegInitSsWaitCnAttachCnfMsgEsmDataReq_ENUM,LNAS_ERROR);
        return NAS_LMM_MSG_DISCARD;
    }

    NAS_EMM_SER_SendMrrcDataReq_ESMdata(&pstEsmDataReq->stEsmMsg, pstEsmDataReq->ulOpId);

    return NAS_LMM_MSG_HANDLED;
}


VOS_UINT32 NAS_EMM_MsRegSsNormalMsgEsmDataReq(VOS_UINT32  ulMsgId,
                                                   VOS_VOID   *pMsgStru
                               )
{
    VOS_UINT32                          ulRslt          = NAS_EMM_FAIL;
    EMM_ESM_DATA_REQ_STRU              *pstEsmDataReq   = (EMM_ESM_DATA_REQ_STRU*)pMsgStru;

    (VOS_VOID)ulMsgId;

    NAS_EMM_SER_LOG_INFO("NAS_EMM_MsRegSsNormalMsgEsmDataReq entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegSsNormalMsgEsmDataReq_ENUM,LNAS_ENTRY);

    /* 函数输入指针参数检查, 状态匹配检查,若不匹配,退出*/
    ulRslt = NAS_EMM_SER_CHKFSMStateMsgp(EMM_MS_REG,EMM_SS_REG_NORMAL_SERVICE,pMsgStru);
    if ( NAS_EMM_SUCC != ulRslt )
    {
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_TAUSER_CHKFSMStateMsgp ERROR !!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsNormalMsgEsmDataReq_ENUM,LNAS_ERROR);
        return NAS_LMM_MSG_DISCARD;
    }

    if (VOS_TRUE == pstEsmDataReq->ulIsEmcType)
    {
        NAS_LMM_SetEmmInfoIsEmerPndEsting(VOS_TRUE);
        /* 解决LRRC REL搜小区驻留上报系统消息前收到ESM紧急承载建立请求，由于空口发送失败，本地detach,发起紧急ATTACH问题
           方案:先高优先级缓存，等到收到LRRC系统消息后处理*/
        if((NAS_EMM_CONN_WAIT_SYS_INFO == NAS_EMM_GetConnState())
            ||(NAS_EMM_CONN_RELEASING == NAS_EMM_GetConnState()))
        {
            return NAS_LMM_STORE_HIGH_PRIO_MSG;
        }
    }

    if((NAS_EMM_CONN_WAIT_SYS_INFO == NAS_EMM_GetConnState())
        ||(NAS_EMM_CONN_RELEASING == NAS_EMM_GetConnState()))
    {
        return NAS_LMM_STORE_HIGH_PRIO_MSG;
    }


    if(NAS_LMM_TIMER_RUNNING == NAS_LMM_IsStaTimerRunning(TI_NAS_EMM_STATE_T3440))
    {
        /* 如果是紧急类型的，则停止T3440定时器，主动发链路释放 */
        if (VOS_TRUE == pstEsmDataReq->ulIsEmcType)
        {
            NAS_LMM_SetEmmInfoIsEmerPndEsting(VOS_TRUE);
            NAS_LMM_StopStateTimer(TI_NAS_EMM_STATE_T3440);

            /*启动定时器TI_NAS_EMM_MRRC_WAIT_RRC_REL*/
            NAS_LMM_StartStateTimer(TI_NAS_EMM_MRRC_WAIT_RRC_REL_CNF);

            /*向MRRC发送NAS_EMM_MRRC_REL_REQ消息*/
            NAS_EMM_SndRrcRelReq(NAS_LMM_NOT_BARRED);

            /* 设置连接状态为释放过程中 */
            NAS_EMM_SetConnState(NAS_EMM_CONN_RELEASING);

        }
        return NAS_LMM_STORE_HIGH_PRIO_MSG;

    }


    /*CONN态，转发ESM消息*/
    if((NAS_EMM_CONN_SIG == NAS_EMM_GetConnState()) ||
        (NAS_EMM_CONN_DATA == NAS_EMM_GetConnState()))
    {
        NAS_EMM_SER_SendMrrcDataReq_ESMdata(&pstEsmDataReq->stEsmMsg, pstEsmDataReq->ulOpId);
        return NAS_LMM_MSG_HANDLED;
    }

    if((NAS_LMM_TIMER_RUNNING == NAS_LMM_IsPtlTimerRunning(TI_NAS_EMM_PTL_CSFB_DELAY))
        &&(VOS_TRUE == NAS_EMM_SER_IsCsfbProcedure()))
    {
        NAS_EMM_SER_LOG_INFO( "NAS_EMM_MsRegSsNormalMsgEsmDataReq: Msg discard, CSFB delay timer is running.");
        TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegSsNormalMsgEsmDataReq_ENUM,LNAS_FUNCTION_LABEL1);
        NAS_EMM_SendEsmDataCnf(EMM_ESM_SEND_RSLT_EMM_DISCARD, pstEsmDataReq->ulOpId);
        return NAS_LMM_MSG_HANDLED;
    }

    NAS_EMM_SER_RcvEsmDataReq(pMsgStru);

    return NAS_LMM_MSG_HANDLED;

}

/*****************************************************************************
 Function Name   : NAS_EMM_MsRegSsNormalMsgSmsEstReq
 Description     : 正常服务状态下处理SMS建链请求
 Input           : None
 Output          : None
 Return          : VOS_UINT32

 History         :
    1.sunbing 49683      2011-11-3  Draft Enact

*****************************************************************************/
VOS_UINT32  NAS_EMM_MsRegSsNormalMsgSmsEstReq
(
    VOS_UINT32  ulMsgId,
    VOS_VOID   *pMsgStru
)
{
    VOS_UINT32                          ulRslt          = NAS_EMM_FAIL;

    (VOS_VOID)ulMsgId;

    NAS_EMM_SER_LOG_INFO("NAS_EMM_MsRegSsNormalMsgSmsEstReq entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegSsNormalMsgSmsEstReq_ENUM,LNAS_ENTRY);

    ulRslt = NAS_EMM_SER_CHKFSMStateMsgp(EMM_MS_REG,EMM_SS_REG_NORMAL_SERVICE,pMsgStru);
    if ( NAS_EMM_SUCC != ulRslt )
    {
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_TAUSER_CHKFSMStateMsgp ERROR !!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsNormalMsgSmsEstReq_ENUM,LNAS_ERROR);
        return NAS_LMM_MSG_DISCARD;
    }

    /*因为建链函数已经经过预处理，进入这个函数，说明CS域已经注册，
      如果不处于空闲态，可以直接回复建链成功*/
    if (NAS_EMM_NO == NAS_LMM_IsRrcConnectStatusIdle())
    {
        NAS_LMM_SndLmmSmsEstCnf();

        NAS_LMM_SetConnectionClientId(NAS_LMM_CONNECTION_CLIENT_ID_SMS);
        return NAS_LMM_MSG_HANDLED;
    }
    if( NAS_EMM_NO == NAS_EMM_IsSerConditionSatisfied())
    {
        NAS_LMM_SndLmmSmsErrInd(LMM_SMS_ERR_CAUSE_OTHERS);
        return NAS_LMM_MSG_HANDLED;
    }
    /* 大数据: 清Mt Ser Flag标志 */
    NAS_EMM_SetOmMtSerFlag(NAS_EMM_NO);
    /*设置SER触发原因为 NAS_EMM_SER_START_CAUSE_SMS_EST_REQ*/
    NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_SMS_EST_REQ);

    /*Inform RABM that SER init*/
    NAS_EMM_SER_SendRabmReestInd(EMM_ERABM_REEST_STATE_INITIATE);

    /*启动定时器3417*/
    NAS_LMM_StartStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417);

    /*转换EMM状态机MS_SER_INIT+SS_SER_WAIT_CN_CNF*/
    NAS_EMM_TAUSER_FSMTranState(EMM_MS_SER_INIT, EMM_SS_SER_WAIT_CN_SER_CNF, TI_NAS_EMM_STATE_SERVICE_T3417);

    /*组合并发送MRRC_DATA_REQ(SERVICE_REQ)*/
    NAS_EMM_SER_SendMrrcDataReq_ServiceReq();

    return NAS_LMM_MSG_HANDLED;
}

#if (FEATURE_LPP == FEATURE_ON)

VOS_UINT32  NAS_EMM_MsRegSsNormalMsgLppEstReq
(
    VOS_UINT32  ulMsgId,
    VOS_VOID   *pMsgStru
)
{
    VOS_UINT32                          ulRslt          = NAS_EMM_FAIL;

    (VOS_VOID)ulMsgId;

    NAS_EMM_SER_LOG_INFO("NAS_EMM_MsRegSsNormalMsgLppEstReq entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegSsNormalMsgLppEstReq_ENUM, LNAS_LPP_Func_Enter);

    ulRslt = NAS_EMM_SER_CHKFSMStateMsgp(EMM_MS_REG,EMM_SS_REG_NORMAL_SERVICE,pMsgStru);
    if ( NAS_EMM_SUCC != ulRslt )
    {
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_TAUSER_CHKFSMStateMsgp ERROR !!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsNormalMsgLppEstReq_ENUM, LNAS_LPP_CheckStatusError);
        return NAS_LMM_MSG_DISCARD;
    }


    /*如果不处于空闲态，可以直接回复建链成功*/
    if (NAS_EMM_NO == NAS_LMM_IsRrcConnectStatusIdle())
    {
        NAS_EMM_SendLppEstCnf(LMM_LPP_EST_RESULT_SUCC);

        return NAS_LMM_MSG_HANDLED;
    }

    if( NAS_EMM_NO == NAS_EMM_IsSerConditionSatisfied())
    {
        NAS_EMM_SndLppCnf(  ID_LPP_LMM_EST_REQ,
                            LMM_LPP_EST_RESULT_FAIL_OTHERS,
                            LMM_LPP_SEND_RSLT_FAIL_3411_RUNNING,
                            LMM_LPP_SEND_RSLT_OHTERS);

        return NAS_LMM_MSG_HANDLED;
    }

    /*设置SER触发原因为 NAS_EMM_SER_START_CAUSE_LPP_EST_REQ*/
    NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_LPP_EST_REQ);

    /*Inform RABM that SER init*/
    NAS_EMM_SER_SendRabmReestInd(EMM_ERABM_REEST_STATE_INITIATE);

    /*启动定时器3417*/
    NAS_LMM_StartStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417);

    /*转换EMM状态机MS_SER_INIT+SS_SER_WAIT_CN_CNF*/
    NAS_EMM_TAUSER_FSMTranState(EMM_MS_SER_INIT, EMM_SS_SER_WAIT_CN_SER_CNF, TI_NAS_EMM_STATE_SERVICE_T3417);

    /*组合并发送MRRC_DATA_REQ(SERVICE_REQ)*/
    NAS_EMM_SER_SendMrrcDataReq_ServiceReq();

    return NAS_LMM_MSG_HANDLED;
}


VOS_UINT32  NAS_EMM_MsRegSsRegAttemptUpdateMmMsgLppEstReq
(
    VOS_UINT32  ulMsgId,
    VOS_VOID   *pMsgStru
)
{
    VOS_UINT32                          ulRslt          = NAS_EMM_FAIL;

    (VOS_VOID)ulMsgId;

    NAS_EMM_SER_LOG_INFO("NAS_EMM_MsRegSsRegAttemptUpdateMmMsgLppEstReq entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegSsRegAttemptUpdateMmMsgLppEstReq_ENUM, LNAS_LPP_Func_Enter);

    ulRslt = NAS_EMM_SER_CHKFSMStateMsgp(EMM_MS_REG,EMM_SS_REG_ATTEMPTING_TO_UPDATE_MM,pMsgStru);
    if ( NAS_EMM_SUCC != ulRslt )
    {
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_TAUSER_CHKFSMStateMsgp ERROR !!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsRegAttemptUpdateMmMsgLppEstReq_ENUM, LNAS_LPP_CheckStatusError);
        return NAS_LMM_MSG_DISCARD;
    }


    /*如果不处于空闲态，可以直接回复建链成功*/
    if (NAS_EMM_NO == NAS_LMM_IsRrcConnectStatusIdle())
    {
        NAS_EMM_SendLppEstCnf(LMM_LPP_EST_RESULT_SUCC);

        return NAS_LMM_MSG_HANDLED;
    }
    if( NAS_EMM_NO == NAS_EMM_IsSerConditionSatisfied())
    {
        NAS_EMM_SndLppCnf(  ID_LPP_LMM_EST_REQ,
                            LMM_LPP_EST_RESULT_FAIL_OTHERS,
                            LMM_LPP_SEND_RSLT_FAIL_3411_RUNNING,
                            LMM_LPP_SEND_RSLT_OHTERS);

        return NAS_LMM_MSG_HANDLED;
    }
    /*设置SER触发原因为 NAS_EMM_SER_START_CAUSE_SMS_EST_REQ*/
    NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_LPP_EST_REQ);

    /*Inform RABM that SER init*/
    NAS_EMM_SER_SendRabmReestInd(EMM_ERABM_REEST_STATE_INITIATE);

    /*启动定时器3417*/
    NAS_LMM_StartStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417);

    /*转换EMM状态机MS_SER_INIT+SS_SER_WAIT_CN_CNF*/
    NAS_EMM_TAUSER_FSMTranState(EMM_MS_SER_INIT, EMM_SS_SER_WAIT_CN_SER_CNF, TI_NAS_EMM_STATE_SERVICE_T3417);

    /*组合并发送MRRC_DATA_REQ(SERVICE_REQ)*/
    NAS_EMM_SER_SendMrrcDataReq_ServiceReq();

    return NAS_LMM_MSG_HANDLED;
}
#endif


VOS_UINT32  NAS_EMM_MsRegSsRegAttemptUpdateMmMsgEsmDataReq
(
    VOS_UINT32  ulMsgId,
    VOS_VOID   *pMsgStru
)
{
    VOS_UINT32                          ulRslt          = NAS_EMM_FAIL;
    EMM_ESM_DATA_REQ_STRU              *pstEsmDataReq   = (EMM_ESM_DATA_REQ_STRU*)pMsgStru;

    (VOS_VOID)ulMsgId;

    NAS_EMM_SER_LOG_INFO("NAS_EMM_MsRegSsRegAttemptUpdateMmMsgEsmDataReq entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegSsRegAttemptUpdateMmMsgEsmDataReq_ENUM,LNAS_ENTRY);

    /* 函数输入指针参数检查, 状态匹配检查,若不匹配,退出*/
    ulRslt = NAS_EMM_SER_CHKFSMStateMsgp(EMM_MS_REG,
                                        EMM_SS_REG_ATTEMPTING_TO_UPDATE_MM,
                                        pMsgStru);
    if ( NAS_EMM_SUCC != ulRslt )
    {
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_TAUSER_CHKFSMStateMsgp ERROR !!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsRegAttemptUpdateMmMsgEsmDataReq_ENUM,LNAS_ERROR);
        return NAS_LMM_MSG_DISCARD;
    }

    if (VOS_TRUE == pstEsmDataReq->ulIsEmcType)
    {
        NAS_LMM_SetEmmInfoIsEmerPndEsting(VOS_TRUE);
        /* 解决LRRC REL搜小区驻留上报系统消息前收到ESM紧急承载建立请求，由于空口发送失败，本地detach,发起紧急ATTACH问题
           方案:先高优先级缓存，等到收到LRRC系统消息后处理*/
        if((NAS_EMM_CONN_WAIT_SYS_INFO == NAS_EMM_GetConnState())
            ||(NAS_EMM_CONN_RELEASING == NAS_EMM_GetConnState()))
        {
            return NAS_LMM_STORE_HIGH_PRIO_MSG;
        }
    }

    if((NAS_EMM_CONN_WAIT_SYS_INFO == NAS_EMM_GetConnState())
        ||(NAS_EMM_CONN_RELEASING == NAS_EMM_GetConnState()))
    {
        return NAS_LMM_STORE_HIGH_PRIO_MSG;
    }

    if(NAS_LMM_TIMER_RUNNING == NAS_LMM_IsStaTimerRunning(TI_NAS_EMM_STATE_T3440))
    {
        /* 如果是紧急类型的，则停止T3440定时器，主动发链路释放 */
        if (VOS_TRUE == pstEsmDataReq->ulIsEmcType)
        {
            NAS_LMM_SetEmmInfoIsEmerPndEsting(VOS_TRUE);
            NAS_LMM_StopStateTimer(TI_NAS_EMM_STATE_T3440);

            /*启动定时器TI_NAS_EMM_MRRC_WAIT_RRC_REL*/
            NAS_LMM_StartStateTimer(TI_NAS_EMM_MRRC_WAIT_RRC_REL_CNF);

            /*向MRRC发送NAS_EMM_MRRC_REL_REQ消息*/
            NAS_EMM_SndRrcRelReq(NAS_LMM_NOT_BARRED);

            /* 设置连接状态为释放过程中 */
            NAS_EMM_SetConnState(NAS_EMM_CONN_RELEASING);

        }
        return NAS_LMM_STORE_HIGH_PRIO_MSG;
    }

    /*CONN态，转发ESM消息*/
    if((NAS_EMM_CONN_SIG == NAS_EMM_GetConnState()) ||
        (NAS_EMM_CONN_DATA == NAS_EMM_GetConnState()))
    {
        NAS_EMM_SER_SendMrrcDataReq_ESMdata(&pstEsmDataReq->stEsmMsg, pstEsmDataReq->ulOpId);
        return NAS_LMM_MSG_HANDLED;
    }
    /* 记录UPDATE_MM标识 */
    /*NAS_LMM_SetEmmInfoUpdateMmFlag(NAS_EMM_UPDATE_MM_FLAG_VALID);*/
    NAS_EMM_SER_RcvEsmDataReq(pMsgStru);

    return NAS_LMM_MSG_HANDLED;
}



VOS_UINT32 NAS_EMM_MsTauInitMsgRabmReestReq(VOS_UINT32  ulMsgId,
                                                   VOS_VOID   *pMsgStru
                                )
{
    VOS_UINT32                      ulRslt                = NAS_EMM_FAIL;

    (VOS_VOID)ulMsgId;

    /* 打印进入该函数， INFO_LEVEL */
    NAS_EMM_SER_LOG_INFO( "Nas_Emm_MsTauInitMsgRabmReestReq is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsTauInitMsgRabmReestReq_ENUM,LNAS_ENTRY);

    /* 函数输入指针参数检查, 状态匹配检查,若不匹配,退出*/
    ulRslt = NAS_EMM_SER_CHKFSMStateMsgp(EMM_MS_TAU_INIT,EMM_SS_TAU_WAIT_CN_TAU_CNF,pMsgStru);
    if ( NAS_EMM_SUCC != ulRslt )
    {
        /* 打印异常 */
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_TAUSER_CHKFSMStateMsgp ERROR !!");

        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsTauInitMsgRabmReestReq_ENUM,LNAS_ERROR);
        return NAS_LMM_MSG_DISCARD;
    }

    return NAS_LMM_MSG_DISCARD;
    }


VOS_UINT32 NAS_EMM_MsTauInitMsgRrcPagingInd(VOS_UINT32  ulMsgId,
                                                   VOS_VOID   *pMsgStru
                                )
{
    VOS_UINT32                      ulRslt              = NAS_EMM_FAIL;

    (VOS_VOID)ulMsgId;

    /* 打印进入该函数， INFO_LEVEL */
    NAS_EMM_SER_LOG_INFO( "Nas_Emm_MsTauInitMsgRrcPagingInd is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsTauInitMsgRrcPagingInd_ENUM,LNAS_ENTRY);

    /* 函数输入指针参数检查, 状态匹配检查,若不匹配,退出*/
    ulRslt = NAS_EMM_SER_CHKFSMStateMsgp(EMM_MS_TAU_INIT,EMM_SS_TAU_WAIT_CN_TAU_CNF,pMsgStru);
    if ( NAS_EMM_SUCC != ulRslt )
    {
        /* 打印异常 */
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_TAUSER_CHKFSMStateMsgp ERROR !!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsTauInitMsgRrcPagingInd_ENUM,LNAS_ERROR);
        return NAS_LMM_MSG_DISCARD;
    }

    return NAS_LMM_STORE_HIGH_PRIO_MSG;
}


VOS_UINT32 NAS_EMM_MsSerInitMsgEsmdataReq
(
    VOS_UINT32                          ulMsgId,
    VOS_VOID                           *pMsgStru
)
{
    VOS_UINT32                          ulRslt             = NAS_EMM_FAIL;
    EMM_ESM_DATA_REQ_STRU              *pstsmdatareq       = NAS_EMM_NULL_PTR;

    (VOS_VOID)ulMsgId;

    /* 打印进入该函数， INFO_LEVEL */
    NAS_EMM_SER_LOG_INFO( "NAS_EMM_MsSerInitMsgEsmdataReq is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsSerInitMsgEsmdataReq_ENUM,LNAS_ENTRY);

    /* 函数输入指针参数检查, 状态匹配检查,若不匹配,退出*/
    ulRslt = NAS_EMM_SER_CHKFSMStateMsgp(EMM_MS_SER_INIT ,EMM_SS_SER_WAIT_CN_SER_CNF,pMsgStru);
    if ( NAS_EMM_SUCC != ulRslt )
    {
        /* 打印异常 */
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_TAUSER_CHKFSMStateMsgp ERROR !!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsSerInitMsgEsmdataReq_ENUM,LNAS_ERROR);
        return NAS_LMM_MSG_DISCARD;
    }

    /*SER模块自行缓存ESM DATA消息*/
    pstsmdatareq = (EMM_ESM_DATA_REQ_STRU        *)pMsgStru;

    if (VOS_TRUE == pstsmdatareq->ulIsEmcType)
    {
        NAS_LMM_SetEmmInfoIsEmerPndEsting(VOS_TRUE);
        /* 缓存紧急类型的ESM消息 */
        NAS_EMM_SaveEmcEsmMsg(pMsgStru);
    }
    NAS_EMM_SER_SaveEsmMsg(pstsmdatareq);

    return  NAS_LMM_MSG_HANDLED;
}

VOS_UINT32 NAS_EMM_MsTauInitMsgEsmdataReq
(
    VOS_UINT32                          ulMsgId,
    VOS_VOID                           *pMsgStru
)
{
    VOS_UINT32                          ulRslt             = NAS_EMM_FAIL;
    EMM_ESM_DATA_REQ_STRU              *pstsmdatareq       = NAS_EMM_NULL_PTR;

    (VOS_VOID)ulMsgId;

    /* 打印进入该函数， INFO_LEVEL */
    NAS_EMM_SER_LOG_INFO( "Nas_Emm_MsTauInitMsgEsmdataReq is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsTauInitMsgEsmdataReq_ENUM,LNAS_ENTRY);

    /* 函数输入指针参数检查, 状态匹配检查,若不匹配,退出*/
    ulRslt = NAS_EMM_SER_CHKFSMStateMsgp(EMM_MS_TAU_INIT,EMM_SS_TAU_WAIT_CN_TAU_CNF,pMsgStru);
    if ( NAS_EMM_SUCC != ulRslt )
    {
        /* 打印异常 */
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_TAUSER_CHKFSMStateMsgp ERROR !!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsTauInitMsgEsmdataReq_ENUM,LNAS_ERROR);
        return NAS_LMM_MSG_DISCARD;
    }

    pstsmdatareq = (EMM_ESM_DATA_REQ_STRU *)pMsgStru;

    /* 当前处于建链过程中时高优先级缓存, 等待建链完成处理, 解决TAU建链过程中承载
       连接以及去连接时间过长问题 */
    if(NAS_EMM_CONN_ESTING == NAS_EMM_GetConnState())
    {
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_MsTauInitMsgEsmdataReq High Prio !!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsTauInitMsgEsmdataReq_ENUM,LNAS_FUNCTION_LABEL1);
        return  NAS_LMM_STORE_HIGH_PRIO_MSG;
    }

    /* 紧急承载建立, 保存紧急ESM消息, 修改TAU发起条件为EMC PDN REQ, 在TAU结束时
       继续处理(TAU 成功时透传ESM消息(可能会存在当前TAU没带ACTIVE标志, ESM消息发
       送时网侧把链路释放了), TAU失败时发起紧急注册流程) */
    if (VOS_TRUE == pstsmdatareq->ulIsEmcType)
    {
        /* 设置标志 */
        NAS_LMM_SetEmmInfoIsEmerPndEsting(VOS_TRUE);

        /* 缓存紧急类型的ESM消息 */
        NAS_EMM_SaveEmcEsmMsg(pMsgStru);

        /* 修改TAU发起标志 */
        NAS_EMM_TAU_SaveEmmTAUStartCause(NAS_EMM_TAU_START_CAUSE_ESM_EMC_PDN_REQ);

        return NAS_LMM_MSG_HANDLED;
    }

    /* 若当前TAU是打断了SERVICE的TAU类型, 则将此收到的ESM消息缓存, 其他情况先低优
       先级缓存当TAU完成后处理 */
    if (NAS_EMM_COLLISION_SERVICE == NAS_EMM_TAU_GetEmmCollisionCtrl())
    {
        /*SER模块自行缓存ESM DATA消息*/
        NAS_EMM_SER_SaveEsmMsg(pstsmdatareq);
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsTauInitMsgEsmdataReq_ENUM,LNAS_FUNCTION_LABEL2);
        return  NAS_LMM_MSG_HANDLED;
    }

    TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsTauInitMsgEsmdataReq_ENUM,LNAS_FUNCTION_LABEL3);
    return NAS_LMM_STORE_LOW_PRIO_MSG;
}


VOS_UINT32  NAS_EMM_MsTauInitMsgRabmRelReq(
    VOS_UINT32                          ulMsgId,
    VOS_VOID                           *pMsgStru
)
{
    NAS_EMM_SER_LOG_INFO( "NAS_EMM_MsTauInitMsgRabmRelReq is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsTauInitMsgRabmRelReq_ENUM,LNAS_ENTRY);
    (VOS_VOID)ulMsgId;
    (VOS_VOID)pMsgStru;

    /*终止当前TAU流程*/
    NAS_LMM_StopStateTimer(TI_NAS_EMM_STATE_TAU_T3430);

    #if (FEATURE_ON == FEATURE_DSDS)
    /*发送end notify消息给RRC，通知RRC释放资源*/
    NAS_EMM_TAU_SendRrcDsdsEndNotify();
    #endif

#if (FEATURE_MULTI_MODEM == FEATURE_ON)
    NAS_EMM_SendMtcTAUEndType();
#endif

    NAS_EMM_TAU_GetEmmTAUAttemptCnt()++;

    NAS_EMM_TAU_ProcAbnormal();

    if (NAS_EMM_TAU_START_CAUSE_ESM_EMC_PDN_REQ == NAS_EMM_TAU_GetEmmTAUStartCause())
    {
        NAS_EMM_TAU_LOG_INFO("NAS_EMM_MsTauInitMsgRabmRelReq:CAUSE_ESM_EMC_PDN_REQ");
        TLPS_PRINT2LAYER_INFO(NAS_EMM_MsTauInitMsgRabmRelReq_ENUM,LNAS_FUNCTION_LABEL1);

        NAS_EMM_EmcPndReqTauAbnormalCommProc(   NAS_EMM_MmcSendTauActionResultIndFailWithPara,
                                                (VOS_VOID*)NAS_EMM_NULL_PTR,
                                                EMM_SS_DEREG_NORMAL_SERVICE);
    }
    else
    {
        NAS_EMM_TAU_RelIndCollisionProc(NAS_EMM_MmcSendTauActionResultIndFailWithPara,
                        (VOS_VOID*)NAS_EMM_NULL_PTR);
    }

    /*向MRRC发送NAS_EMM_MRRC_REL_REQ消息*/
    NAS_EMM_RelReq(                     NAS_LMM_NOT_BARRED);



    return  NAS_LMM_MSG_HANDLED;
}



VOS_VOID    NAS_EMM_SER_RcvRabmReestReq
(
    VOS_UINT32                          ulIsEmcType
)
{
    NAS_EMM_SER_LOG_INFO( "Nas_Emm_Ser_RcvRabmReestReq is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_RcvRabmReestReq_ENUM,LNAS_ENTRY);
    if (VOS_TRUE == ulIsEmcType)
    {
        NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_RABM_REEST_EMC);
    }
    else
    {
        if( NAS_EMM_NO == NAS_EMM_IsSerConditionSatisfied())
        {
            NAS_EMM_SER_SendRabmReestInd(EMM_ERABM_REEST_STATE_FAIL);
            return;
        }
        NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_RABM_REEST);
    }
    /* 大数据: 清Mt Ser Flag标志 */
    NAS_EMM_SetOmMtSerFlag(NAS_EMM_NO);
    /*Inform RABM that SER init*/
    NAS_EMM_SER_SendRabmReestInd(EMM_ERABM_REEST_STATE_INITIATE);

    /*启动定时器3417*/
    NAS_LMM_StartStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417);

    /*转换EMM状态机MS_SER_INIT+SS_SER_WAIT_CN_CNF*/
    NAS_EMM_TAUSER_FSMTranState(EMM_MS_SER_INIT, EMM_SS_SER_WAIT_CN_SER_CNF, TI_NAS_EMM_STATE_SERVICE_T3417);

    /*组合并发送MRRC_DATA_REQ(SERVICE_REQ)*/
    NAS_EMM_SER_SendMrrcDataReq_ServiceReq();
    return;
}


VOS_VOID    NAS_EMM_SER_RcvRrcStmsiPagingInd(VOS_VOID)
{

    NAS_EMM_SER_LOG_INFO( "NAS_EMM_SER_RcvRrcStmsiPagingInd is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_RcvRrcStmsiPagingInd_ENUM,LNAS_ENTRY);

    /* 大数据: 设置SER类型为MT Ser */
    NAS_EMM_SetOmMtSerFlag(NAS_EMM_YES);
    /* 大数据: 记录Mt Ser流程启动时间 */
    NAS_EMM_SaveMtSerStartTimeStamp();
    /*设置SER触发原因为 NAS_EMM_SER_START_CAUSE_RRC_PAGING*/
    NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_RRC_PAGING);

    /*Inform RABM that SER init*/
    NAS_EMM_SER_SendRabmReestInd(EMM_ERABM_REEST_STATE_INITIATE);

    /*启动定时器3417*/
    NAS_LMM_StartStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417);

    /*转换EMM状态机MS_SER_INIT+SS_SER_WAIT_CN_CNF*/
    NAS_EMM_TAUSER_FSMTranState(EMM_MS_SER_INIT, EMM_SS_SER_WAIT_CN_SER_CNF, TI_NAS_EMM_STATE_SERVICE_T3417);

    /*组合并发送MRRC_DATA_REQ(SERVICE_REQ)*/
    NAS_EMM_SER_SendMrrcDataReq_ServiceReq();
    return;

}


VOS_UINT32 NAS_EMM_SER_CsDomainNotRegProcNormalCsfb(VOS_VOID)
{
    NAS_EMM_SER_LOG_WARN("NAS_EMM_SER_CsDomainNotRegProcNormalCsfb is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_CsDomainNotRegProcNormalCsfb_ENUM, LNAS_ENTRY);

    /* 被#2拒绝过，直接终止 */
    if (NAS_EMM_REJ_YES == NAS_LMM_GetEmmInfoRejCause2Flag())
    {
        NAS_EMM_SER_LOG_NORM("NAS_EMM_SER_CsDomainNotRegProcNormalCsfb:REJ#2!");
        TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_CsDomainNotRegProcNormalCsfb_ENUM, LNAS_END);

        NAS_EMM_MmSendCsfbSerEndInd(MM_LMM_CSFB_SERVICE_RSLT_VERIFY_CSFB_FAIL_FOR_OTHERS,
                                        NAS_LMM_CAUSE_NULL);
        return NAS_LMM_MSG_HANDLED;
    }

    /* UPDATE MM态进状态机处理 */
    if ((EMM_MS_REG == NAS_LMM_GetEmmCurFsmMS())
      &&(EMM_SS_REG_ATTEMPTING_TO_UPDATE_MM == NAS_LMM_GetEmmCurFsmSS()))
    {
        return NAS_LMM_MSG_DISCARD;
    }

    /* 其他状态搜网去GU */
    NAS_EMM_MmcSendSerResultIndOtherType(MMC_LMM_SERVICE_RSLT_FAILURE);
    NAS_EMM_RelReq(NAS_LMM_NOT_BARRED);
    return NAS_LMM_MSG_HANDLED;

}


VOS_UINT32 NAS_EMM_SER_CheckCsfbNeedHighPrioStore(VOS_VOID)
{
    if (NAS_EMM_CONN_ESTING == NAS_EMM_GetConnState())
    {
        NAS_EMM_SER_LOG_NORM("NAS_EMM_SER_CheckCsfbNeedHighPrioStore:Esting, High priority storage!");
        TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_CheckCsfbNeedHighPrioStore_ENUM, LNAS_FUNCTION_LABEL1);
        return NAS_EMM_SUCC;
    }

    /* T3440定时器启动期间不再高优先级缓存，改为启动delay定时器，防止出现网络一直不发释放导致的发起呼叫慢问题 */
    if ((NAS_EMM_CONN_RELEASING == NAS_EMM_GetConnState())
      ||(NAS_LMM_TIMER_RUNNING == NAS_LMM_IsPtlTimerRunning(TI_NAS_EMM_PTL_REATTACH_DELAY)))
    {
        NAS_EMM_SER_LOG_NORM("NAS_EMM_SER_CheckCsfbNeedHighPrioStore:Releasing, High priority storage!");
        TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_CheckCsfbNeedHighPrioStore_ENUM, LNAS_FUNCTION_LABEL2);
        return NAS_EMM_SUCC;
    }

    NAS_EMM_SER_LOG_NORM("NAS_EMM_SER_CheckCsfbNeedHighPrioStore:Don't need high prio store.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_CheckCsfbNeedHighPrioStore_ENUM, LNAS_FUNCTION_LABEL3);
    return NAS_EMM_FAIL;
}


VOS_UINT32 NAS_EMM_SER_CheckCsfbNeedLowPrioStore
(
    MM_LMM_CSFB_SERVICE_TYPE_ENUM_UINT32  enCsfbSrvTyp
)
{
    switch(NAS_EMM_CUR_MAIN_STAT)
    {
        case    EMM_MS_TAU_INIT:
        case    EMM_MS_AUTH_INIT:
        case    EMM_MS_RESUME:

                NAS_EMM_SER_LOG_NORM("NAS_EMM_SER_CheckCsfbNeedLowPrioStore:TAU, need store!");
                TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_CheckCsfbNeedLowPrioStore_ENUM, LNAS_FUNCTION_LABEL1);
                return NAS_EMM_SUCC;

        case    EMM_MS_SER_INIT:
                if ((NAS_EMM_SER_START_CAUSE_MO_CSFB_REQ == NAS_EMM_SER_GetEmmSERStartCause())
                    &&(MM_LMM_CSFB_SERVICE_MO_NORMAL == enCsfbSrvTyp))
                {
                    return NAS_EMM_FAIL;
                }

                /* 当前与已经存在的MO CSFB冲突，缓存 */
                if (NAS_EMM_SER_START_CAUSE_MT_CSFB_REQ != NAS_EMM_SER_GetEmmSERStartCause())
                {
                    NAS_EMM_SER_LOG_NORM("NAS_EMM_SER_CheckCsfbNeedLowPrioStore:SER, need store!");
                    TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_CheckCsfbNeedLowPrioStore_ENUM, LNAS_FUNCTION_LABEL2);
                    return NAS_EMM_SUCC;
                }

                break;

        case    EMM_MS_REG:

                if (EMM_SS_REG_IMSI_DETACH_WATI_CN_DETACH_CNF == NAS_LMM_GetEmmCurFsmSS())
                {
                    NAS_EMM_SER_LOG_NORM("NAS_EMM_SER_CheckCsfbNeedLowPrioStore:Imsi detach, need store!");
                    TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_CheckCsfbNeedLowPrioStore_ENUM, LNAS_FUNCTION_LABEL3);
                    return NAS_EMM_SUCC;
                }

                break;

        case    EMM_MS_REG_INIT:

                if ((EMM_SS_ATTACH_WAIT_RRC_DATA_CNF == NAS_LMM_GetEmmCurFsmSS())
                ||(EMM_SS_ATTACH_WAIT_ESM_BEARER_CNF == NAS_LMM_GetEmmCurFsmSS()))
                {
                    NAS_EMM_SER_LOG_NORM("NAS_EMM_SER_CheckCsfbNeedLowPrioStore:Wait rrc data cnf and wait esm bearer cnf,need store!");
                    TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_CheckCsfbNeedLowPrioStore_ENUM, LNAS_FUNCTION_LABEL4);
                    return NAS_EMM_SUCC;
                }

                break;

        default:
            break;
    }

    NAS_EMM_SER_LOG_NORM("NAS_EMM_SER_CheckCsfbNeedLowPrioStore:Don't need store.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_CheckCsfbNeedLowPrioStore_ENUM, LNAS_FUNCTION_LABEL4);
    return NAS_EMM_FAIL;
}


VOS_UINT32 NAS_EMM_SER_VerifyNormalCsfb(VOS_VOID)
{
    NAS_LMM_CS_SERVICE_ENUM_UINT32      ulCsService = NAS_LMM_CS_SERVICE_BUTT;

    /* 不是CS_PS UE mode返回失败 */
    if (NAS_EMM_YES != NAS_EMM_IsCsPsUeMode())
    {
        NAS_EMM_SER_LOG_NORM("NAS_EMM_SER_VerifyNormalCsfb:Not CS_PS UE mode!");
        TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_VerifyNormalCsfb_ENUM, LNAS_FUNCTION_LABEL1);
        return NAS_EMM_FAIL;
    }

    /* 判断UE是否支持CSFB,如果CS SERVICE未使能，则默认支持CSFB */
    ulCsService = NAS_EMM_GetCsService();
    if ((NAS_LMM_CS_SERVICE_CSFB_SMS != ulCsService)
      &&(NAS_LMM_CS_SERVICE_BUTT != ulCsService))
    {
        NAS_EMM_SER_LOG_NORM("NAS_EMM_SER_VerifyNormalCsfb: UE is not support csfb!");
        TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_VerifyNormalCsfb_ENUM, LNAS_FUNCTION_LABEL2);
        return NAS_EMM_FAIL;
    }

    /* 判断是否是L单模 */
    if (NAS_EMM_SUCC != NAS_EMM_CheckMutiModeSupport())
    {
        NAS_EMM_SER_LOG_NORM("NAS_EMM_SER_VerifyNormalCsfb: Lte only");
        TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_VerifyNormalCsfb_ENUM, LNAS_FUNCTION_LABEL3);
        return NAS_EMM_FAIL;
    }

    return NAS_EMM_SUCC;
}


VOS_UINT32 NAS_EMM_SER_VerifyMtCsfb( VOS_VOID )
{
    NAS_LMM_CS_SERVICE_ENUM_UINT32      ulCsService = NAS_LMM_CS_SERVICE_BUTT;

    /* MT CALL时不判断注册域:解决GU2L,L上TAU由于底层异常,导致TAU失败转到ATTEMP_TO_UPDATE_MM态时收到CS PAGING不处理问题 */

    /* 判断UE是否支持CSFB,如果CS SERVICE未使能，则默认支持CSFB */
    ulCsService = NAS_EMM_GetCsService();
    if ((NAS_LMM_CS_SERVICE_CSFB_SMS != ulCsService)
        && (NAS_LMM_CS_SERVICE_BUTT != ulCsService))
    {
        NAS_EMM_SER_LOG_NORM( "NAS_EMM_SER_VerifyMtCsfb:ue is not support csfb!");
        TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_VerifyMtCsfb_ENUM,LNAS_FUNCTION_LABEL1);
        return NAS_EMM_FAIL;
    }

    /* 删除原不用的注释代码，和SMS ONLY的相关判断 */
    #if (FEATURE_ON == FEATURE_PTM)
    if (NAS_LMM_ADDITIONAL_UPDATE_SMS_ONLY == NAS_EMM_GetAddUpdateRslt())
    {
        NAS_EMM_ImprovePerformceeErrReport(EMM_OM_ERRLOG_IMPROVEMENT_TYPE_CS_MT_CALL);
    }
    #endif

    /* 判断是否是L单模 */
    if(NAS_EMM_SUCC != NAS_EMM_CheckMutiModeSupport())
    {
        NAS_EMM_SER_LOG_NORM( "NAS_EMM_SER_VerifyMtCsfb:lte only");
        TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_VerifyMtCsfb_ENUM,LNAS_FUNCTION_LABEL3);
        return NAS_EMM_FAIL;
    }

    return NAS_EMM_SUCC;
}


VOS_UINT32 NAS_EMM_SER_VerifyCsfb(MM_LMM_CSFB_SERVICE_TYPE_ENUM_UINT32  enCsfbSrvType)
{

    NAS_LMM_CS_SERVICE_ENUM_UINT32      ulCsService = NAS_LMM_CS_SERVICE_BUTT;

    /* 判断注册域是否为CS+PS */
    if (NAS_LMM_REG_DOMAIN_CS_PS != NAS_LMM_GetEmmInfoRegDomain())
    {
       NAS_EMM_SER_LOG_NORM( "NAS_EMM_SER_VerifyCsfb:cs is not registered!");
       TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_VerifyCsfb_ENUM,LNAS_FUNCTION_LABEL1);
       return NAS_EMM_FAIL;
    }

    /* 判断UE是否支持CSFB,如果CS SERVICE未使能，则默认支持CSFB */
    ulCsService = NAS_EMM_GetCsService();
    if ((NAS_LMM_CS_SERVICE_CSFB_SMS != ulCsService)
       && (NAS_LMM_CS_SERVICE_BUTT != ulCsService))
    {
       NAS_EMM_SER_LOG_NORM( "NAS_EMM_SER_VerifyCsfb:ue is not support csfb!");\
       TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_VerifyCsfb_ENUM,LNAS_FUNCTION_LABEL2);
       return NAS_EMM_FAIL;
    }

    /* 对于MO类型的，进入稳态后处理，对于紧急类型的，在预处理里面已经判断，
    应该不会出现，在调用分支中注意 */
    /* 判断网侧是否携带了SMS ONLY */
    if ((NAS_LMM_ADDITIONAL_UPDATE_SMS_ONLY == NAS_EMM_GetAddUpdateRslt())
        && (MM_LMM_CSFB_SERVICE_MT_NORMAL == enCsfbSrvType))
    {
       NAS_EMM_SER_LOG_NORM( "NAS_EMM_SER_VerifyCsfb:additional update result sms only");
       TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_VerifyCsfb_ENUM,LNAS_FUNCTION_LABEL3);
       return NAS_EMM_FAIL;
    }

    /* 判断是否是L单模 */
    if(NAS_EMM_SUCC != NAS_EMM_CheckMutiModeSupport())
    {
       NAS_EMM_SER_LOG_NORM( "NAS_EMM_SER_VerifyCsfb:lte only");
       TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_VerifyCsfb_ENUM,LNAS_FUNCTION_LABEL4);
       return NAS_EMM_FAIL;
    }
    return NAS_EMM_SUCC;
}



VOS_VOID    NAS_EMM_SER_RcvRrcCsPagingInd
(
    LRRC_LNAS_PAGING_UE_ID_ENUM_UINT32 enPagingUeId
)
{

    NAS_EMM_SER_LOG_INFO( "NAS_EMM_SER_RcvRrcCsPagingInd is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_RcvRrcCsPagingInd_ENUM,LNAS_ENTRY);

    /* 检测MT CSFB流程是否能够发起 */
    if (NAS_EMM_FAIL == NAS_EMM_SER_VerifyMtCsfb())
    {
        NAS_EMM_SER_LOG_NORM( "NAS_EMM_SER_RcvRrcCsPagingInd:cannot csfb!");
        TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_RcvRrcCsPagingInd_ENUM,LNAS_FUNCTION_LABEL1);
        return ;
    }

    /* 给MM模块发送MM_MM_CSFB_SERVICE_PAGING_IND消息 */
    NAS_EMM_MmSendCsfbSerPaingInd(  NAS_EMM_MT_CSFB_TYPE_CS_PAGING,
                                    VOS_NULL_PTR,
                                    enPagingUeId);
    return;
}


VOS_VOID    NAS_EMM_SER_RcvEsmDataReq(VOS_VOID   *pMsgStru)
{
    EMM_ESM_DATA_REQ_STRU        *pstsmdatareq = (EMM_ESM_DATA_REQ_STRU*)pMsgStru;

    NAS_EMM_SER_LOG_INFO( "Nas_Emm_Ser_RcvEsmDataReq is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_RcvEsmDataReq_ENUM,LNAS_ENTRY);
    /* 设置SERVICE触发原因值 */
    if (VOS_TRUE == pstsmdatareq->ulIsEmcType)
    {
        NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_ESM_DATA_REQ_EMC);

        /* 缓存紧急类型的ESM消息 */
        NAS_EMM_SaveEmcEsmMsg(          pMsgStru);
    }
    else
    {
        if( NAS_EMM_NO == NAS_EMM_IsSerConditionSatisfied())
        {
            NAS_EMM_SendEsmDataCnf(EMM_ESM_SEND_RSLT_EMM_DISCARD, pstsmdatareq->ulOpId);
            return;
        }
        NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_ESM_DATA_REQ);
    }
    /* 大数据:　清Mt Ser Flag */
    NAS_EMM_SetOmMtSerFlag(NAS_EMM_NO);
    /*Inform RABM that SER init*/
    NAS_EMM_SER_SendRabmReestInd(EMM_ERABM_REEST_STATE_INITIATE);

    /*SER模块自行缓存ESM DATA消息*/
    pstsmdatareq = (EMM_ESM_DATA_REQ_STRU        *)pMsgStru;
    NAS_EMM_SER_SaveEsmMsg(pstsmdatareq);

    /*启动定时器3417*/
    NAS_LMM_StartStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417);

    /*转换EMM状态机MS_SER_INIT+SS_SER_WAIT_CN_CNF*/
    NAS_EMM_TAUSER_FSMTranState(EMM_MS_SER_INIT, EMM_SS_SER_WAIT_CN_SER_CNF, TI_NAS_EMM_STATE_SERVICE_T3417);

    /*组合并发送MRRC_DATA_REQ(SERVICE_REQ)*/
    NAS_EMM_SER_SendMrrcDataReq_ServiceReq();
    return;
}


VOS_VOID  NAS_EMM_SER_UplinkPending( VOS_VOID )
{
    NAS_EMM_SER_LOG_INFO( "NAS_EMM_SER_UplinkPending is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_UplinkPending_ENUM,LNAS_ENTRY);

    if( NAS_EMM_NO == NAS_EMM_IsSerConditionSatisfied())
    {
        return;
    }

    /* 大数据: 清Mt Ser Flag */
    NAS_EMM_SetOmMtSerFlag(NAS_EMM_NO);
    /*设置SER触发原因为 NAS_ESM_SER_START_CAUSE_UPLINK_PENDING*/
    NAS_EMM_SER_SaveEmmSERStartCause(NAS_ESM_SER_START_CAUSE_UPLINK_PENDING);

    /*Inform RABM that SER init*/
    NAS_EMM_SER_SendRabmReestInd(EMM_ERABM_REEST_STATE_INITIATE);

    /*启动定时器3417*/
    NAS_LMM_StartStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417);

    /*转换EMM状态机MS_SER_INIT+SS_SER_WAIT_CN_CNF*/
    NAS_EMM_TAUSER_FSMTranState(EMM_MS_SER_INIT, EMM_SS_SER_WAIT_CN_SER_CNF, TI_NAS_EMM_STATE_SERVICE_T3417);

     /*组合并发送MRRC_DATA_REQ(SERVICE_REQ)*/
    NAS_EMM_SER_SendMrrcDataReq_ServiceReq();
    return;
}



VOS_VOID  NAS_EMM_SER_SmsEstReq( VOS_VOID )
{
    NAS_EMM_SER_LOG_INFO( "NAS_EMM_SER_SmsEstReq is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_SmsEstReq_ENUM,LNAS_ENTRY);

    if( NAS_EMM_NO == NAS_EMM_IsSerConditionSatisfied())
    {
        NAS_LMM_SndLmmSmsErrInd(LMM_SMS_ERR_CAUSE_OTHERS);
        return;
    }

    /*设置SER触发原因为 NAS_EMM_SER_START_CAUSE_SMS_EST_REQ*/
    NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_SMS_EST_REQ);

    /*Inform RABM that SER init*/
    NAS_EMM_SER_SendRabmReestInd(EMM_ERABM_REEST_STATE_INITIATE);

    /*组合并发送MRRC_DATA_REQ(SERVICE_REQ)*/
    NAS_EMM_SER_SendMrrcDataReq_ServiceReq();

    /*启动定时器3417*/
    NAS_LMM_StartStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417);

    /*转换EMM状态机MS_SER_INIT+SS_SER_WAIT_CN_CNF*/
    NAS_EMM_TAUSER_FSMTranState(EMM_MS_SER_INIT, EMM_SS_SER_WAIT_CN_SER_CNF, TI_NAS_EMM_STATE_SERVICE_T3417);

    return;
}




/*lint -e960*/
/*lint -e961*/
VOS_BOOL NAS_EMM_SER_IsSameEsmMsgInBuf
(
    const EMM_ESM_DATA_REQ_STRU               *pMsgStru
)
{
    VOS_UINT32                          i       = 0;
    EMM_ESM_DATA_REQ_STRU              *pEsmMsg = NAS_LMM_NULL_PTR;

    /* 如果消息长度和内容相同，就认为是重复消息 */
    for (i = 0; i < g_stEmmEsmMsgBuf.ulEsmMsgCnt; i++)
    {
        if (NAS_LMM_NULL_PTR != g_stEmmEsmMsgBuf.apucEsmMsgBuf[i])
        {
            pEsmMsg = (EMM_ESM_DATA_REQ_STRU *)g_stEmmEsmMsgBuf.apucEsmMsgBuf[i];

            if ((pMsgStru->stEsmMsg.ulEsmMsgSize == pEsmMsg->stEsmMsg.ulEsmMsgSize)
             && (0 == NAS_LMM_MEM_CMP(pMsgStru->stEsmMsg.aucEsmMsg,
                                     pEsmMsg->stEsmMsg.aucEsmMsg,
                                     pEsmMsg->stEsmMsg.ulEsmMsgSize))
             && (pMsgStru->ulOpId == pEsmMsg->ulOpId)
             && (pMsgStru->ulIsEmcType == pEsmMsg->ulIsEmcType)
               )
            {
                return VOS_TRUE;
            }
        }
        else
        {
            NAS_EMM_SER_LOG_WARN( "NAS_EMM_SER_IsSameEsmMsgInBuf: Null buffer item.");
            TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_IsSameEsmMsgInBuf_ENUM,LNAS_FUNCTION_LABEL1);
        }
    }

    return VOS_FALSE;
}


VOS_UINT32 NAS_EMM_SER_FindEsmMsg
(
    VOS_UINT32                          ulOpid
)
{
    VOS_UINT32                          ulIndex     = NAS_EMM_NULL;
    EMM_ESM_DATA_REQ_STRU              *pstEsmMsg   = NAS_EMM_NULL_PTR;

    /* 打印进入该函数， INFO_LEVEL */
    NAS_EMM_SER_LOG_INFO( "NAS_EMM_SER_FindEsmMsg is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_FindEsmMsg_ENUM,LNAS_ENTRY);

    for (ulIndex = NAS_EMM_NULL; ulIndex < g_stEmmEsmMsgBuf.ulEsmMsgCnt; ulIndex++)
    {
        pstEsmMsg = (EMM_ESM_DATA_REQ_STRU *)g_stEmmEsmMsgBuf.apucEsmMsgBuf[ulIndex];
        if (ulOpid == pstEsmMsg->ulOpId)
        {
            return ulIndex;
        }
    }

    NAS_EMM_SER_LOG_INFO( "NAS_EMM_SER_FindEsmMsg failed!");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_FindEsmMsg_ENUM,LNAS_FAIL);

    return NAS_EMM_SER_MAX_ESM_BUFF_MSG_NUM;
}


VOS_VOID NAS_EMM_SER_DeleteEsmMsg
(
    VOS_UINT32                          ulOpid
)
{
    VOS_UINT32                          ulIndex = NAS_EMM_NULL;
    VOS_UINT32                          ulRslt  = NAS_EMM_NULL;

    /* 打印进入该函数， INFO_LEVEL */
    NAS_EMM_SER_LOG_INFO( "NAS_EMM_SER_DeleteEsmMsg is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_DeleteEsmMsg_ENUM,LNAS_ENTRY);

    ulIndex = NAS_EMM_SER_FindEsmMsg(ulOpid);
    if (ulIndex >= g_stEmmEsmMsgBuf.ulEsmMsgCnt)
    {
        return ;
    }

    ulRslt = NAS_COMM_FreeBuffItem(NAS_COMM_BUFF_TYPE_EMM, g_stEmmEsmMsgBuf.apucEsmMsgBuf[ulIndex]);

    if (NAS_COMM_BUFF_SUCCESS != ulRslt)
    {
       NAS_EMM_SER_LOG_WARN("NAS_EMM_SER_DeleteEsmMsg, Memory Free is not succ");
       TLPS_PRINT2LAYER_WARNING(NAS_EMM_SER_DeleteEsmMsg_ENUM,LNAS_FUNCTION_LABEL1);
    }

    g_stEmmEsmMsgBuf.apucEsmMsgBuf[ulIndex] = NAS_LMM_NULL_PTR;

    for (; ulIndex < (g_stEmmEsmMsgBuf.ulEsmMsgCnt - 1); ulIndex++)
    {
        g_stEmmEsmMsgBuf.apucEsmMsgBuf[ulIndex] = g_stEmmEsmMsgBuf.apucEsmMsgBuf[ulIndex+1];
    }

    g_stEmmEsmMsgBuf.apucEsmMsgBuf[g_stEmmEsmMsgBuf.ulEsmMsgCnt - 1] = NAS_LMM_NULL_PTR;

    g_stEmmEsmMsgBuf.ulEsmMsgCnt--;
}


VOS_VOID NAS_EMM_SER_SaveEsmMsg(const EMM_ESM_DATA_REQ_STRU  *pMsgStru)
{
    VOS_VOID                            *pMsgBuf   = NAS_LMM_NULL_PTR;
    VOS_UINT32                           ulBufSize = 0;

    /* 打印进入该函数， INFO_LEVEL */
    NAS_EMM_SER_LOG_INFO( "NAS_EMM_Ser_SaveEsmMsg is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_SaveEsmMsg_ENUM,LNAS_ENTRY);

    /* 不是重复的SM消息，插入队列*/
    if ((VOS_FALSE == NAS_EMM_SER_IsSameEsmMsgInBuf(pMsgStru))
     && (NAS_EMM_SER_MAX_ESM_BUFF_MSG_NUM > g_stEmmEsmMsgBuf.ulEsmMsgCnt))
    {
        ulBufSize = pMsgStru->stEsmMsg.ulEsmMsgSize +
                    sizeof(pMsgStru->stEsmMsg.ulEsmMsgSize) +
                    sizeof(pMsgStru->ulOpId) +
                    sizeof(pMsgStru->ulIsEmcType) +
                    EMM_LEN_VOS_MSG_HEADER +
                    EMM_LEN_MSG_ID;

        /* 分配空间 */
        pMsgBuf = NAS_COMM_AllocBuffItem(NAS_COMM_BUFF_TYPE_EMM, ulBufSize);

        if (NAS_LMM_NULL_PTR != pMsgBuf)
        {
            NAS_LMM_MEM_CPY_S(pMsgBuf, ulBufSize, pMsgStru, ulBufSize);

            g_stEmmEsmMsgBuf.apucEsmMsgBuf[g_stEmmEsmMsgBuf.ulEsmMsgCnt] = pMsgBuf;
            g_stEmmEsmMsgBuf.ulEsmMsgCnt++;

        }
        else
        {
            NAS_EMM_SER_LOG_INFO( "NAS_EMM_Ser_SaveEsmMsg: NAS_AllocBuffItem return null pointer.");
            TLPS_PRINT2LAYER_INFO(NAS_EMM_SER_SaveEsmMsg_ENUM,LNAS_FUNCTION_LABEL1);
        }
    }
    else
    {
        NAS_EMM_SER_LOG1_INFO( "NAS_EMM_Ser_SaveEsmMsg: ESM Msg Not Buffered, Buffered msg number is:",
                               g_stEmmEsmMsgBuf.ulEsmMsgCnt);
        TLPS_PRINT2LAYER_INFO1(NAS_EMM_SER_SaveEsmMsg_ENUM,LNAS_FUNCTION_LABEL2,
                               g_stEmmEsmMsgBuf.ulEsmMsgCnt);
    }

    NAS_EMM_SER_LOG1_INFO( "NAS_EMM_Ser_SaveEsmMsg: Buffered msg number is:",
                           g_stEmmEsmMsgBuf.ulEsmMsgCnt);
    TLPS_PRINT2LAYER_INFO1(NAS_EMM_SER_SaveEsmMsg_ENUM,LNAS_FUNCTION_LABEL3,
                               g_stEmmEsmMsgBuf.ulEsmMsgCnt);

    return;
}



VOS_UINT32 NAS_EMM_EmmMsRegInitSsWaitRrcDataCnfMsgEsmDataReq
(
    VOS_UINT32                          ulMsgId,
    VOS_VOID                           *pMsgStru
)
{
    EMM_ESM_DATA_REQ_STRU              *pstEsmDataReq = (EMM_ESM_DATA_REQ_STRU*)pMsgStru;

    (VOS_VOID)(ulMsgId);
    NAS_EMM_SER_LOG_INFO("NAS_EMM_EmmMsRegInitSsWaitRrcDataCnfMsgEsmDataReq is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_EmmMsRegInitSsWaitRrcDataCnfMsgEsmDataReq_ENUM,LNAS_ENTRY);
    NAS_EMM_SER_SendMrrcDataReq_ESMdata(&pstEsmDataReq->stEsmMsg, pstEsmDataReq->ulOpId);
    return NAS_LMM_MSG_HANDLED;
}




VOS_VOID  NAS_EMM_MsTauSerSsWaitCnCnfEmergencyCsfbProc(VOS_VOID)
{
    MMC_LMM_TAU_RSLT_ENUM_UINT32        ulTauRslt = MMC_LMM_TAU_RSLT_BUTT;

    /* TAU过程中, 后面挂起会自动清除资源 */
    if (EMM_MS_TAU_INIT == NAS_LMM_GetEmmCurFsmMS())
    {
        /* 向MMC发送LMM_MMC_TAU_RESULT_IND消息 */
        ulTauRslt = MMC_LMM_TAU_RSLT_FAILURE;
        NAS_EMM_MmcSendTauActionResultIndOthertype((VOS_VOID*)&ulTauRslt);

        NAS_EMM_TAU_AbnormalOver();
    }
    else  /* SER过程中,终止SER */
    {
        NAS_EMM_SER_AbnormalOver();
    }

    /* 转入REG.PLMN-SEARCH等MMC挂起 */
    NAS_EMM_AdStateConvert(EMM_MS_REG,
                           EMM_SS_REG_PLMN_SEARCH,
                           TI_NAS_EMM_STATE_NO_TIMER);

    /*向MMC发送LMM_MMC_SERVICE_RESULT_IND消息*/
    NAS_EMM_MmcSendSerResultIndOtherType(MMC_LMM_SERVICE_RSLT_FAILURE);

    /* 如果处于连接态，压栈释放处理 */
    NAS_EMM_RelReq(NAS_LMM_NOT_BARRED);

    return;

}


VOS_VOID  NAS_EMM_MsAnySsWaitCnDetachCnfEmergencyCsfbProc(VOS_VOID)
{
    NAS_LMM_StopStateTimer(TI_NAS_EMM_T3421);

    #if (FEATURE_ON == FEATURE_DSDS)
    /*发送end notify消息给RRC，通知RRC释放资源*/
    NAS_LMM_SendRrcDsdsEndNotify(LRRC_LNAS_SESSION_TYPE_PS_DETACH);
    #endif

    /* REG. EMM_SS_REG_IMSI_DETACH_WATI_CN_DETACH_CNF 态*/
    if (EMM_MS_REG == NAS_LMM_GetEmmCurFsmMS())
    {
        NAS_LMM_SetEmmInfoRegDomain(NAS_LMM_REG_DOMAIN_PS);

        /* 向MMC发送本地LMM_MMC_DETACH_IND消息 */
        NAS_EMM_SendDetRslt(MMC_LMM_DETACH_RSLT_SUCCESS);
        NAS_EMM_AdStateConvert(EMM_MS_REG,
                               EMM_SS_REG_NORMAL_SERVICE,
                               TI_NAS_EMM_STATE_NO_TIMER);

        NAS_LMM_ImsiDetachReleaseResource();
    }
    else /* DEREG_INIT. EMM_SS_DETACH_WAIT_CN_DETACH_CNF 态*/
    {
        /*向MMC发送本地LMM_MMC_DETACH_IND消息*/
        NAS_EMM_SendDetRslt(MMC_LMM_DETACH_RSLT_SUCCESS);
        NAS_EMM_AdStateConvert(EMM_MS_DEREG,
                               EMM_SS_DEREG_NORMAL_SERVICE,
                               TI_NAS_EMM_STATE_NO_TIMER);

        /* 通知ESM清除资源 */
        NAS_EMM_TAU_SendEsmStatusInd(EMM_ESM_ATTACH_STATUS_DETACHED);
    }

    /*向MMC发送LMM_MMC_SERVICE_RESULT_IND消息*/
    NAS_EMM_MmcSendSerResultIndOtherType(MMC_LMM_SERVICE_RSLT_FAILURE);

    /* 如果处于连接态，压栈释放处理 */
    NAS_EMM_RelReq(NAS_LMM_NOT_BARRED);

    return;

}


VOS_VOID  NAS_EMM_MsRegInitSsAnyStateEmergencyCsfbProc(VOS_VOID)
{
    /* 给MMC上报ATTACH结果为失败 */
    NAS_EMM_AppSendAttOtherType(MMC_LMM_ATT_RSLT_FAILURE);

    /* ATTACH停定时器 + 清除资源 */
    NAS_EMM_Attach_SuspendInitClearResourse();

    /* 修改状态：进入主状态DEREG子状态DEREG_PLMN_SEARCH, 此时服务状态不上报改变*/
    NAS_EMM_AdStateConvert(EMM_MS_DEREG,
                           EMM_SS_DEREG_PLMN_SEARCH ,
                           TI_NAS_EMM_STATE_NO_TIMER);

    /* 通知ESM清除资源 */
    NAS_EMM_TAU_SendEsmStatusInd(EMM_ESM_ATTACH_STATUS_DETACHED);

    /*向MMC发送LMM_MMC_SERVICE_RESULT_IND消息*/
    NAS_EMM_MmcSendSerResultIndOtherType(MMC_LMM_SERVICE_RSLT_FAILURE);

    /* 如果处于连接态，压栈释放处理 */
    NAS_EMM_RelReq(NAS_LMM_NOT_BARRED);

    return;

}


VOS_UINT32  NAS_EMM_UnableDirectlyStartMoEmergencyCsfbProc(VOS_VOID)
{

    if(NAS_EMM_CONN_ESTING == NAS_EMM_GetConnState())
    {
        return  NAS_LMM_STORE_HIGH_PRIO_MSG;
    }

    if( (NAS_EMM_CONN_RELEASING == NAS_EMM_GetConnState())
        || (NAS_LMM_TIMER_RUNNING == NAS_LMM_IsStaTimerRunning(TI_NAS_EMM_STATE_T3440))
        || (NAS_LMM_TIMER_RUNNING == NAS_LMM_IsPtlTimerRunning(TI_NAS_EMM_PTL_REATTACH_DELAY)))
    {
        NAS_EMM_SER_LOG_NORM( "NAS_EMM_CannotDirectlyStartMoEmergencyCsfbProc:High priority storage!");
        TLPS_PRINT2LAYER_INFO(NAS_EMM_UnableDirectlyStartMoEmergencyCsfbProc_ENUM,LNAS_FUNCTION_LABEL1);
        return  NAS_LMM_STORE_HIGH_PRIO_MSG;
    }

    switch(NAS_EMM_CUR_MAIN_STAT)
    {
        case    EMM_MS_REG_INIT:
            /* 设置SERVICE发起原因为紧急CSFB, 用于给MMC上报SERVICE_RESULT_IND */
            NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_MO_EMERGENCY_CSFB_REQ);
            NAS_EMM_MsRegInitSsAnyStateEmergencyCsfbProc();
            break;

        case    EMM_MS_TAU_INIT:
        case    EMM_MS_SER_INIT:
            /* 设置SERVICE发起原因为紧急CSFB, 用于给MMC上报SERVICE_RESULT_IND */
            NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_MO_EMERGENCY_CSFB_REQ);
            NAS_EMM_MsTauSerSsWaitCnCnfEmergencyCsfbProc();
            break;

        case    EMM_MS_AUTH_INIT:
        case    EMM_MS_RESUME:
            NAS_EMM_SER_LOG_NORM( "NAS_EMM_CannotDirectlyStartMoEmergencyCsfbProc:Low priority storage!");
            TLPS_PRINT2LAYER_INFO(NAS_EMM_UnableDirectlyStartMoEmergencyCsfbProc_ENUM,LNAS_FUNCTION_LABEL2);
            return NAS_LMM_STORE_LOW_PRIO_MSG;

        case    EMM_MS_REG:
        case    EMM_MS_DEREG_INIT:
            /* 设置SERVICE发起原因为紧急CSFB, 用于给MMC上报SERVICE_RESULT_IND */
            NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_MO_EMERGENCY_CSFB_REQ);
            if ((EMM_SS_REG_IMSI_DETACH_WATI_CN_DETACH_CNF == NAS_LMM_GetEmmCurFsmSS())
                || (EMM_SS_DETACH_WAIT_CN_DETACH_CNF == NAS_LMM_GetEmmCurFsmSS()))
            {
                NAS_EMM_MsAnySsWaitCnDetachCnfEmergencyCsfbProc();
            }
            else
            {
                /*向MMC发送LMM_MMC_SERVICE_RESULT_IND消息*/
                NAS_EMM_MmcSendSerResultIndOtherType(MMC_LMM_SERVICE_RSLT_FAILURE);
                NAS_EMM_RelReq(NAS_LMM_NOT_BARRED);
            }
            break;

        default:
            /* 设置SERVICE发起原因为紧急CSFB, 用于给MMC上报SERVICE_RESULT_IND */
            NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_MO_EMERGENCY_CSFB_REQ);
            /*向MMC发送LMM_MMC_SERVICE_RESULT_IND消息*/
            NAS_EMM_MmcSendSerResultIndOtherType(MMC_LMM_SERVICE_RSLT_FAILURE);
            NAS_EMM_RelReq(NAS_LMM_NOT_BARRED);
            break;
    }

    return NAS_LMM_MSG_HANDLED;

}


VOS_UINT32  NAS_EMM_MsRegPreProcMmNormalCsfbNotifyMsg
(
    MM_LMM_CSFB_SERVICE_TYPE_ENUM_UINT32  enCsfbSrvTyp
)
{
    NAS_EMM_SER_LOG_NORM( "NAS_EMM_MsRegPreProcMmNormalCsfbNotifyMsg:enter!");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegPreProcMmNormalCsfbNotifyMsg_ENUM,LNAS_ENTRY);

    switch (NAS_LMM_GetEmmCurFsmSS())
    {
        /* 这些状态都进状态机处理 */
        case    EMM_SS_REG_NORMAL_SERVICE         :
        case    EMM_SS_REG_WAIT_ACCESS_GRANT_IND  :
        case    EMM_SS_REG_ATTEMPTING_TO_UPDATE_MM:

                return NAS_LMM_MSG_DISCARD;

        /* 受限服务检查IMS电话是否存在，不存在搜网去GU */
        case    EMM_SS_REG_LIMITED_SERVICE        :

                /* 如果有IMS电话，终止CSFB，否则触发搜网去GU */
                #if (FEATURE_ON == FEATURE_IMS)
                if (VOS_TRUE == IMSA_IsCallConnExist())
                {
                    NAS_EMM_SER_LOG_WARN("NAS_EMM_MsRegPreProcMmNormalCsfbNotifyMsg:Limit service,IMS call!");
                    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegPreProcMmNormalCsfbNotifyMsg_ENUM, LNAS_FUNCTION_LABEL1);
                    NAS_EMM_MmSendCsfbSerEndInd(MM_LMM_CSFB_SERVICE_RSLT_VERIFY_CSFB_FAIL_FOR_OTHERS,
                                        NAS_LMM_CAUSE_NULL);
                }
                else
                #endif
                {
                    /* 无论是MO还是MT，都去GU搜网，紧急CSFB目前不会走到这里 */
                    NAS_EMM_MmcSendSerResultIndOtherType(MMC_LMM_SERVICE_RSLT_FAILURE);
                    NAS_EMM_RelReq(NAS_LMM_NOT_BARRED);
                }
                #if (FEATURE_ON == FEATURE_PTM)
                if (MM_LMM_CSFB_SERVICE_MT_NORMAL == enCsfbSrvTyp)
                {
                    NAS_EMM_ImprovePerformceeErrReport(EMM_OM_ERRLOG_IMPROVEMENT_TYPE_CS_MT_CALL);
                }
                #endif
                break;

        /* 这三个状态直接搜网去GU */
        case    EMM_SS_REG_ATTEMPTING_TO_UPDATE   :
        case    EMM_SS_REG_PLMN_SEARCH            :
        case    EMM_SS_REG_NO_CELL_AVAILABLE      :

                NAS_EMM_MmcSendSerResultIndOtherType(MMC_LMM_SERVICE_RSLT_FAILURE);

                #if (FEATURE_ON == FEATURE_PTM)
                if (MM_LMM_CSFB_SERVICE_MO_NORMAL == enCsfbSrvTyp)
                {
                    NAS_EMM_ImprovePerformceeErrReport(EMM_OM_ERRLOG_IMPROVEMENT_TYPE_CS_MO_CALL);
                }
                else
                {
                    NAS_EMM_ImprovePerformceeErrReport(EMM_OM_ERRLOG_IMPROVEMENT_TYPE_CS_MT_CALL);
                }
                #endif
                break;

        /* 只剩余1个子状态IMSI DETACH，前面已经缓存了 */
        default:
                break;
    }

    return NAS_LMM_MSG_HANDLED;

}



VOS_UINT32  NAS_EMM_RcvMmNormalCsfbNotifyMsgProc
(
    MM_LMM_CSFB_SERVICE_TYPE_ENUM_UINT32          enCsfbSrvTyp
)
{
    NAS_EMM_SER_LOG_NORM("NAS_EMM_RcvMmNormalCsfbNotifyMsgProc is entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_RcvMmNormalCsfbNotifyMsgProc_ENUM, LNAS_ENTRY);

    /* 清除CSFB ABORT标识 */
    NAS_EMM_SER_SaveEmmSerCsfbAbortFlag(NAS_EMM_CSFB_ABORT_FLAG_INVALID);

    /* 检测CSFB流程是否能够发起 */
    if (NAS_EMM_FAIL == NAS_EMM_SER_VerifyNormalCsfb())
    {
        NAS_EMM_MmSendCsfbSerEndInd(MM_LMM_CSFB_SERVICE_RSLT_VERIFY_CSFB_FAIL_FOR_OTHERS,
                                        NAS_LMM_CAUSE_NULL);

        return NAS_LMM_MSG_HANDLED;
    }

    /* 检查是否需要高优先级缓存 */
    if (NAS_EMM_SUCC == NAS_EMM_SER_CheckCsfbNeedHighPrioStore())
    {
        return NAS_LMM_STORE_HIGH_PRIO_MSG;
    }
    /* 判断是否需要低优先级缓存 */
    if (NAS_EMM_SUCC == NAS_EMM_SER_CheckCsfbNeedLowPrioStore(enCsfbSrvTyp))
    {
        return NAS_LMM_STORE_LOW_PRIO_MSG;
    }

    /* 注册域不是CS_PS的处理 */
    if (NAS_LMM_REG_DOMAIN_CS_PS != NAS_LMM_GetEmmInfoRegDomain())
    {
        return NAS_EMM_SER_CsDomainNotRegProcNormalCsfb();
    }

    /* 判断网侧是否携带了SMS ONLY，如果是，去GU搜网，如果是连接态需要释放链路 */
    if ((NAS_LMM_ADDITIONAL_UPDATE_SMS_ONLY == NAS_EMM_GetAddUpdateRslt())
      ||(NAS_LMM_TIMER_RUNNING == NAS_LMM_IsStaTimerRunning(TI_NAS_EMM_STATE_SERVICE_T3442)))
    {
        NAS_EMM_SER_LOG_WARN("NAS_EMM_RcvMmNormalCsfbNotifyMsgProc:Sms only or T3442 is running!");
        TLPS_PRINT2LAYER_INFO(NAS_EMM_RcvMmNormalCsfbNotifyMsgProc_ENUM, LNAS_FUNCTION_LABEL1);
        NAS_EMM_MmcSendSerResultIndOtherType(MMC_LMM_SERVICE_RSLT_FAILURE);
        NAS_EMM_RelReq(NAS_LMM_NOT_BARRED);

        #if (FEATURE_ON == FEATURE_PTM)
        if (MM_LMM_CSFB_SERVICE_MT_NORMAL == enCsfbSrvTyp)
        {
            NAS_EMM_ImprovePerformceeErrReport(EMM_OM_ERRLOG_IMPROVEMENT_TYPE_CS_MT_CALL);
        }
        else if (MM_LMM_CSFB_SERVICE_MO_NORMAL == enCsfbSrvTyp)
        {
            NAS_EMM_ImprovePerformceeErrReport(EMM_OM_ERRLOG_IMPROVEMENT_TYPE_CS_MO_CALL);
        }
        else
        {
        }
        #endif
        return NAS_LMM_MSG_HANDLED;
    }

    switch(NAS_EMM_CUR_MAIN_STAT)
    {
        case    EMM_MS_SER_INIT:
                /* 降圈复杂度 */
                return NAS_EMM_MsSerInitPreProcMmNormalCsfbNotifyMsg(enCsfbSrvTyp);

        case    EMM_MS_REG:

                return NAS_EMM_MsRegPreProcMmNormalCsfbNotifyMsg(enCsfbSrvTyp);

        default:/*其他状态为错误的状态，增加告警打印*/
                NAS_EMM_PUBU_LOG1_ERR("NAS_EMM_RcvMmNormalCsfbNotifyMsgProc: Main State is err!",NAS_EMM_CUR_MAIN_STAT);
                TLPS_PRINT2LAYER_ERROR1(NAS_EMM_RcvMmNormalCsfbNotifyMsgProc_ENUM,LNAS_EMM_MAIN_STATE,NAS_EMM_CUR_MAIN_STAT);

                NAS_EMM_SetCsfbProcedureFlag(PS_FALSE);

                /*为容错，增加对MM的回复*/
                NAS_EMM_MmSendCsfbSerEndInd(MM_LMM_CSFB_SERVICE_RSLT_FAILURE, NAS_LMM_CAUSE_NULL);
                break;
    }

    return NAS_LMM_MSG_HANDLED;

}
/*lint +e961*/
/*lint +e960*/

VOS_UINT32  NAS_EMM_RcvMmMoEmergencyCsfbNotifyMsgProc(VOS_VOID)
{
    /* 清除CSFB ABORT标识 */
    NAS_EMM_SER_SaveEmmSerCsfbAbortFlag(NAS_EMM_CSFB_ABORT_FLAG_INVALID);

    /* L单模不能发起紧急CSFB */
    if (NAS_EMM_SUCC != NAS_EMM_CheckMutiModeSupport())
    {
        NAS_EMM_MmSendCsfbSerEndInd(MM_LMM_CSFB_SERVICE_RSLT_VERIFY_CSFB_FAIL_FOR_OTHERS, NAS_LMM_CAUSE_NULL);
        return NAS_LMM_MSG_HANDLED;
    }

    /* 此处置Service启动标志主要是为了当无法发起CSFB时回复MMC Service结果使用，
       若当前主状态为Ser Init，此时置标志会导致Service流程异常 */

    /* 可能可以直接发起紧急CSFB流程, 根据当前所处状态进行不同处理*/
    if (NAS_EMM_SUCC == NAS_EMM_SER_VerifyCsfb(MM_LMM_CSFB_SERVICE_MO_EMERGENCY))
    {
        if((NAS_EMM_CONN_ESTING == NAS_EMM_GetConnState())
            || (NAS_EMM_CONN_RELEASING == NAS_EMM_GetConnState())
            || (NAS_LMM_TIMER_RUNNING == NAS_LMM_IsPtlTimerRunning(TI_NAS_EMM_PTL_REATTACH_DELAY)))
        {
            return  NAS_LMM_STORE_HIGH_PRIO_MSG;
        }

        if(NAS_LMM_TIMER_RUNNING == NAS_LMM_IsStaTimerRunning(TI_NAS_EMM_STATE_T3440))
        {
            NAS_EMM_SER_LOG_NORM( "NAS_EMM_RcvMmMoEmergencyCsfbNotifyMsgProc:High priority storage!");
            TLPS_PRINT2LAYER_INFO(NAS_EMM_RcvMmMoEmergencyCsfbNotifyMsgProc_ENUM,LNAS_FUNCTION_LABEL1);
                if (NAS_RELEASE_R11)
                {
                    /* upon receiving a request from upper layers to establish either a CS emergency call or
                    a PDN connection for emergency bearer services, the UE shall stop timer T3340 and
                    shall locally release the NAS signalling connection, before proceeding as specified in subclause 5.6.1*/
                    /* 如果状态不是RELEASING状态，则之前肯定是启动了T3440定时器 */
                    NAS_LMM_StopStateTimer(TI_NAS_EMM_STATE_T3440);

                    /*启动定时器TI_NAS_EMM_MRRC_WAIT_RRC_REL*/
                    NAS_LMM_StartStateTimer(TI_NAS_EMM_MRRC_WAIT_RRC_REL_CNF);

                    /*向MRRC发送NAS_EMM_MRRC_REL_REQ消息*/
                    NAS_EMM_SndRrcRelReq(NAS_LMM_NOT_BARRED);

                    /* 设置连接状态为释放过程中 */
                    NAS_EMM_SetConnState(NAS_EMM_CONN_RELEASING);
                }
            return NAS_LMM_STORE_HIGH_PRIO_MSG;
        }

        switch(NAS_EMM_CUR_MAIN_STAT)
        {
            case    EMM_MS_TAU_INIT:
            case    EMM_MS_SER_INIT:
            case    EMM_MS_AUTH_INIT:
            case    EMM_MS_RESUME:
                NAS_EMM_SER_LOG_NORM( "NAS_EMM_RcvMmMoEmergencyCsfbNotifyMsgProc:Low priority storage!");
                TLPS_PRINT2LAYER_INFO(NAS_EMM_RcvMmMoEmergencyCsfbNotifyMsgProc_ENUM,LNAS_FUNCTION_LABEL2);
                return NAS_LMM_STORE_LOW_PRIO_MSG;

            /*只有REG+NORMAL_SERVICE态可能直接发起紧急CSFB*/
            case    EMM_MS_REG:
                if (EMM_SS_REG_NORMAL_SERVICE == NAS_EMM_CUR_SUB_STAT)
                {
                    return NAS_LMM_MSG_DISCARD;
                }
                break;

            default:
                break;

        }
    }

    /* 必然不能直接发起或者状态不是上面所列则认为不能发起 */
    return NAS_EMM_UnableDirectlyStartMoEmergencyCsfbProc();

}


VOS_UINT32  NAS_EMM_PreProcMsgMmCsfbSerStartNotify( MsgBlock * pMsg )
{
    MM_LMM_CSFB_SERVICE_START_NOTIFY_STRU *pstCsfbSerStartNotify = VOS_NULL_PTR;

    pstCsfbSerStartNotify = (MM_LMM_CSFB_SERVICE_START_NOTIFY_STRU*)pMsg;

    /* 紧急CSFB的处理 */
    if (MM_LMM_CSFB_SERVICE_MO_EMERGENCY == pstCsfbSerStartNotify->enCsfbSrvType)
    {
        return NAS_EMM_RcvMmMoEmergencyCsfbNotifyMsgProc();
    }
    else  /* MO或MT的NORMAL CSFB的处理 */
    {
        return NAS_EMM_RcvMmNormalCsfbNotifyMsgProc(pstCsfbSerStartNotify->enCsfbSrvType);
    }
}


VOS_UINT32  NAS_EMM_MsRegSsNormalMsgMmCsfbSerStartNotify
(
    VOS_UINT32                          ulMsgId,
    VOS_VOID                           *pMsgStru
)
{
    VOS_UINT32                             ulRslt                   = NAS_EMM_FAIL;
    MM_LMM_CSFB_SERVICE_START_NOTIFY_STRU *pstCsfbSerStartNotify    = VOS_NULL_PTR;
    VOS_UINT32                             ulTimerLen               = NAS_EMM_NULL;

    (VOS_VOID)ulMsgId;

    NAS_EMM_SER_LOG_INFO("NAS_EMM_MsRegSsNormalMsgMmCsfbSerStartNotify entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegSsNormalMsgMmCsfbSerStartNotify_ENUM,LNAS_ENTRY);

    pstCsfbSerStartNotify = (MM_LMM_CSFB_SERVICE_START_NOTIFY_STRU *)pMsgStru;

    ulRslt = NAS_EMM_SER_CHKFSMStateMsgp(EMM_MS_REG,EMM_SS_REG_NORMAL_SERVICE,pMsgStru);
    if ( NAS_EMM_SUCC != ulRslt )
    {
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_MsRegSsNormalMsgMmCsfbSerStartNotify ERROR !!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsNormalMsgMmCsfbSerStartNotify_ENUM,LNAS_ERROR);
        return NAS_LMM_MSG_DISCARD;
    }

    /*设置SER触发原因值*/
    if (MM_LMM_CSFB_SERVICE_MO_NORMAL == pstCsfbSerStartNotify->enCsfbSrvType)
    {
        NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_MO_CSFB_REQ);
    }
    else if (MM_LMM_CSFB_SERVICE_MO_EMERGENCY == pstCsfbSerStartNotify->enCsfbSrvType)
    {
        NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_MO_EMERGENCY_CSFB_REQ);
    }
    else
    {
        NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_MT_CSFB_REQ);
    }

    /* 如果处于释放过程中，因底层处于未驻留状态，启动CSFB延时定时器，等收到系统消息时再考虑发起 */
    if((NAS_EMM_CONN_RELEASING == NAS_EMM_GetConnState())
     ||(NAS_EMM_CONN_WAIT_SYS_INFO == NAS_EMM_GetConnState()) \
     /* 为解决T3440定时器启动期间网络一直不释放链路，导致响应被叫太慢的问题，
     此定时器启动期间不再高优先级缓存，改为启动delay定时器 */
     ||(NAS_LMM_TIMER_RUNNING == NAS_LMM_IsStaTimerRunning(TI_NAS_EMM_STATE_T3440)))
    {
       NAS_LMM_StartPtlTimer(TI_NAS_EMM_PTL_CSFB_DELAY);
       return NAS_LMM_MSG_HANDLED;
    }

    /* 设置UE接受CSFB */
    NAS_EMM_SER_SaveEmmSerCsfbRsp(NAS_EMM_CSFB_RSP_ACCEPTED_BY_UE);

    ulTimerLen = NAS_LMM_GetT3417extTimerLen(NAS_EMM_ESR_CAUSE_TYPE_GUL_CALL);

    NAS_LMM_ModifyStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417_EXT,ulTimerLen);

    /*启动定时器3417*/
    NAS_LMM_StartStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417_EXT);

    /*转换EMM状态机MS_SER_INIT+SS_SER_WAIT_CN_CNF*/
    NAS_EMM_TAUSER_FSMTranState(EMM_MS_SER_INIT, EMM_SS_SER_WAIT_CN_SER_CNF, TI_NAS_EMM_STATE_SERVICE_T3417_EXT);

    /* ESR空口发送前,通知LRRC CSFB流程INIT,LRRC置一个全局变量,当ESR发送时发生重建,直接回LMM REL,搜网去GU */

    NAS_EMM_SndLrrcLmmCsfbNotify(LRRC_LNAS_CSFB_STATUS_INIT);

    /*组合并发送MRRC_DATA_REQ(SERVICE_REQ)*/
    NAS_EMM_SER_SendMrrcDataReq_ExtendedServiceReq();
    return NAS_LMM_MSG_HANDLED;
}


VOS_UINT32  NAS_EMM_MsRegSsWaitAccessGrantIndMsgMmCsfbSerStartNotify
(
    VOS_UINT32                          ulMsgId,
    VOS_VOID                           *pMsgStru
)
{
    VOS_UINT32                             ulRslt                   = NAS_EMM_FAIL;
    MM_LMM_CSFB_SERVICE_START_NOTIFY_STRU *pstCsfbSerStartNotify    = VOS_NULL_PTR;
    VOS_UINT32                             ulTimerLen               = NAS_EMM_NULL;

    (VOS_VOID)ulMsgId;

    NAS_EMM_SER_LOG_INFO("NAS_EMM_MsRegSsWaitAccessGrantIndMsgMmCsfbSerStartNotify entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegSsWaitAccessGrantIndMsgMmCsfbSerStartNotify_ENUM,LNAS_ENTRY);

    pstCsfbSerStartNotify = (MM_LMM_CSFB_SERVICE_START_NOTIFY_STRU *)pMsgStru;

    ulRslt = NAS_EMM_SER_CHKFSMStateMsgp(EMM_MS_REG,EMM_SS_REG_WAIT_ACCESS_GRANT_IND,pMsgStru);
    if ( NAS_EMM_SUCC != ulRslt )
    {
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_MsRegSsWaitAccessGrantIndMsgMmCsfbSerStartNotify ERROR !!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsWaitAccessGrantIndMsgMmCsfbSerStartNotify_ENUM,LNAS_ERROR);
        return NAS_LMM_MSG_DISCARD;
    }

    /* MO CSFB不能发起 */
    if (MM_LMM_CSFB_SERVICE_MO_NORMAL == pstCsfbSerStartNotify->enCsfbSrvType)
    {
        NAS_EMM_MmcSendSerResultIndOtherType(MMC_LMM_SERVICE_RSLT_FAILURE);
        return NAS_LMM_MSG_HANDLED;
    }

    /* 此状态不可能收到紧急CSFB,预处理中已规避 */

    /* 如果MT被BAR，且是MT CSFB，则直接搜网去GU */
    if ((NAS_EMM_SUCC == NAS_EMM_JudgeBarType(NAS_EMM_BAR_TYPE_MT))
      &&(MM_LMM_CSFB_SERVICE_MT_NORMAL == pstCsfbSerStartNotify->enCsfbSrvType))
    {
        NAS_EMM_MmcSendSerResultIndOtherType(MMC_LMM_SERVICE_RSLT_FAILURE);
        #if (FEATURE_ON == FEATURE_PTM)
        /* 被罢状态，收到CS PAGING消息时，不主动上报CHR，等收到CSFB START NOTIFY时再上报 */
        NAS_EMM_ImprovePerformceeErrReport(EMM_OM_ERRLOG_IMPROVEMENT_TYPE_CS_MT_CALL);
        #endif
        return NAS_LMM_MSG_HANDLED;
    }

    /* 设置SER启动原因为MT CSFB */
    NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_MT_CSFB_REQ);

    /* 设置UE接受CSFB */
    NAS_EMM_SER_SaveEmmSerCsfbRsp(NAS_EMM_CSFB_RSP_ACCEPTED_BY_UE);

    /* ESR空口发送前,通知LRRC CSFB流程INIT,LRRC置一个全局变量,当ESR发送时发生重建,直接回LMM REL,搜网去GU */
    NAS_EMM_SndLrrcLmmCsfbNotify(LRRC_LNAS_CSFB_STATUS_INIT);

    ulTimerLen = NAS_LMM_GetT3417extTimerLen(NAS_EMM_ESR_CAUSE_TYPE_GUL_CALL);

    NAS_LMM_ModifyStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417_EXT,ulTimerLen);

    /*启动定时器3417*/
    NAS_LMM_StartStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417_EXT);

    /*转换EMM状态机MS_SER_INIT+SS_SER_WAIT_CN_CNF*/
    NAS_EMM_TAUSER_FSMTranState(EMM_MS_SER_INIT, EMM_SS_SER_WAIT_CN_SER_CNF, TI_NAS_EMM_STATE_SERVICE_T3417_EXT);

    /*组合并发送MRRC_DATA_REQ(SERVICE_REQ)*/
    NAS_EMM_SER_SendMrrcDataReq_ExtendedServiceReq();
    return NAS_LMM_MSG_HANDLED;
}

/* 删除不使用的UPDATE MM态收到CS PAGING函数的定义 */


VOS_UINT32 NAS_EMM_MsSerInitPreProcMmNormalCsfbNotifyMsg
(
    MM_LMM_CSFB_SERVICE_TYPE_ENUM_UINT32  enCsfbSrvTyp
)
{
    if (NAS_EMM_SER_START_CAUSE_MO_CSFB_REQ == NAS_EMM_SER_GetEmmSERStartCause())
    {
        if (MM_LMM_CSFB_SERVICE_MO_NORMAL == enCsfbSrvTyp)
        {
            /*停止定时器T3417ext*/
            NAS_LMM_StopStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417_EXT);

            /*设置SER触发原因值*/
            NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_MO_CSFB_REQ);

            /* 设置UE接受CSFB */
            NAS_EMM_SER_SaveEmmSerCsfbRsp(NAS_EMM_CSFB_RSP_ACCEPTED_BY_UE);

            /* ESR空口发送前,通知LRRC CSFB流程INIT,LRRC置一个全局变量,当ESR发送时发生重建,直接回LMM REL,搜网去GU */

            NAS_EMM_SndLrrcLmmCsfbNotify(LRRC_LNAS_CSFB_STATUS_INIT);

            /*组合并发送MRRC_DATA_REQ(SERVICE_REQ)*/
            NAS_EMM_SER_SendMrrcDataReq_ExtendedServiceReq();

            /*启动定时器T3417ext*/
            NAS_LMM_StartStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417_EXT);

            return NAS_LMM_MSG_HANDLED;
        }
    }

    /* 判断当前是MT CSFB触发的ESR流程中，且网侧再次触发MT CSFB，则重新触发ESR，定时器T3417EXT重新启动
       如果收到了MO类型的CSFB则直接丢弃*/
    if ((NAS_EMM_SER_START_CAUSE_MT_CSFB_REQ == NAS_EMM_SER_GetEmmSERStartCause()))
    {
        if (MM_LMM_CSFB_SERVICE_MT_NORMAL == enCsfbSrvTyp)
        {
            /*停止定时器T3417ext*/
            NAS_LMM_StopStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417_EXT);

            /*设置SER触发原因值*/
            NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_MT_CSFB_REQ);

            /* 设置UE接受CSFB */
            NAS_EMM_SER_SaveEmmSerCsfbRsp(NAS_EMM_CSFB_RSP_ACCEPTED_BY_UE);

            /* ESR空口发送前,通知LRRC CSFB流程INIT,LRRC置一个全局变量,当ESR发送时发生重建,直接回LMM REL,搜网去GU */

            NAS_EMM_SndLrrcLmmCsfbNotify(LRRC_LNAS_CSFB_STATUS_INIT);

            /*组合并发送MRRC_DATA_REQ(SERVICE_REQ)*/
            NAS_EMM_SER_SendMrrcDataReq_ExtendedServiceReq();

            /*启动定时器T3417ext*/
            NAS_LMM_StartStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417_EXT);
        }
    }
    /* 与MO CSFB冲突的场景属于缓存场景，在前面已经缓存 */
    return NAS_LMM_MSG_HANDLED;

}

VOS_UINT32  NAS_EMM_MsRegSsAttempToUpdateMmMsgMmCsfbSerStartNotify
(
    VOS_UINT32                          ulMsgId,
    VOS_VOID                           *pMsgStru
)
{
    VOS_UINT32                             ulRslt                   = NAS_EMM_FAIL;
    MM_LMM_CSFB_SERVICE_START_NOTIFY_STRU *pstCsfbSerStartNotify    = VOS_NULL_PTR;
    VOS_UINT32                             ulTimerLen               = NAS_EMM_NULL;

    (VOS_VOID)ulMsgId;

    NAS_EMM_SER_LOG_INFO("NAS_EMM_MsRegSsAttempToUpdateMmMsgMmCsfbSerStartNotify entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegSsAttempToUpdateMmMsgMmCsfbSerStartNotify_ENUM,LNAS_ENTRY);

    pstCsfbSerStartNotify = (MM_LMM_CSFB_SERVICE_START_NOTIFY_STRU *)pMsgStru;

    ulRslt = NAS_EMM_SER_CHKFSMStateMsgp(EMM_MS_REG,EMM_SS_REG_ATTEMPTING_TO_UPDATE_MM,pMsgStru);
    if ( NAS_EMM_SUCC != ulRslt )
    {
        NAS_EMM_SER_LOG_WARN( "NAS_EMM_MsRegSsAttempToUpdateMmMsgMmCsfbSerStartNotify ERROR !!");
        TLPS_PRINT2LAYER_WARNING(NAS_EMM_MsRegSsAttempToUpdateMmMsgMmCsfbSerStartNotify_ENUM,LNAS_ERROR);
        return NAS_LMM_MSG_DISCARD;
    }

    /*如果是MO CSFB或者紧急CSFB，则直接回复失败，触发MMC选网到GU，如果当前为
      连接态，还需要主动释放链路 */
    if ((MM_LMM_CSFB_SERVICE_MO_NORMAL == pstCsfbSerStartNotify->enCsfbSrvType)
        || (MM_LMM_CSFB_SERVICE_MO_EMERGENCY == pstCsfbSerStartNotify->enCsfbSrvType))
    {
        NAS_EMM_MmcSendSerResultIndOtherType(MMC_LMM_SERVICE_RSLT_FAILURE);

        NAS_EMM_RelReq(NAS_LMM_NOT_BARRED);

        return NAS_LMM_MSG_HANDLED;
    }

    NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_MT_CSFB_REQ);

    /* 如果处于释放过程中，因底层处于未驻留状态，启动CSFB延时定时器，等收到系统消息时再考虑发起，
    为解决T3440定时器启动期间网络一直不释放链路，导致响应被叫太慢的问题，此定时器启动期间不再高
    优先级缓存，改为启动delay定时器 */
    if((NAS_EMM_CONN_RELEASING == NAS_EMM_GetConnState())
     ||(NAS_EMM_CONN_WAIT_SYS_INFO == NAS_EMM_GetConnState())
     ||(NAS_LMM_TIMER_RUNNING == NAS_LMM_IsStaTimerRunning(TI_NAS_EMM_STATE_T3440)))
    {
       NAS_LMM_StartPtlTimer(TI_NAS_EMM_PTL_CSFB_DELAY);
       return NAS_LMM_MSG_HANDLED;
    }

    /* 设置UE接受CSFB */
    NAS_EMM_SER_SaveEmmSerCsfbRsp(NAS_EMM_CSFB_RSP_ACCEPTED_BY_UE);

    ulTimerLen = NAS_LMM_GetT3417extTimerLen(NAS_EMM_ESR_CAUSE_TYPE_GUL_CALL);

    NAS_LMM_ModifyStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417_EXT,ulTimerLen);

    /*启动定时器3417*/
    NAS_LMM_StartStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417_EXT);

    /*转换EMM状态机MS_SER_INIT+SS_SER_WAIT_CN_CNF*/
    NAS_EMM_TAUSER_FSMTranState(EMM_MS_SER_INIT, EMM_SS_SER_WAIT_CN_SER_CNF, TI_NAS_EMM_STATE_SERVICE_T3417_EXT);

    /* ESR空口发送前,通知LRRC CSFB流程INIT,LRRC置一个全局变量,当ESR发送时发生重建,直接回LMM REL,搜网去GU */

    NAS_EMM_SndLrrcLmmCsfbNotify(LRRC_LNAS_CSFB_STATUS_INIT);

    /*组合并发送MRRC_DATA_REQ(SERVICE_REQ)*/
    NAS_EMM_SER_SendMrrcDataReq_ExtendedServiceReq();
    return NAS_LMM_MSG_HANDLED;
}


VOS_UINT32  NAS_EMM_MsRegPreProcXccCallStartNtfMsg
(
    VOS_VOID
)
{
    NAS_EMM_SER_LOG_NORM( "NAS_EMM_MsRegPreProcProcXccCallStartNtfMsg:enter!");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegPreProcProcXccCallStartNtfMsg_ENUM,LNAS_ENTRY);

    switch (NAS_LMM_GetEmmCurFsmSS())
    {
        /* 这些状态都进状态机处理 */
        case    EMM_SS_REG_NORMAL_SERVICE           :
        case    EMM_SS_REG_ATTEMPTING_TO_UPDATE_MM  :

                return NAS_LMM_MSG_DISCARD;

        default:
                NAS_EMM_SER_SaveXccCallFlg(NAS_EMM_XCC_CALL_FLAG_VALID);

                NAS_EMM_ERABM_SendCallStartInd();
                NAS_EMM_XCC_SendEsrEndInd(LMM_XCC_ESR_RSLT_NOT_SEND);
                break;
    }

    return NAS_LMM_MSG_HANDLED;

}


VOS_UINT32 NAS_EMM_PreProcXccCallStartNtf(MsgBlock  *pMsg)
{
    NAS_EMM_EVENT_TYPE_ENUM_UINT32      ulMsgType;
    NAS_LMM_FSM_MSG_BUF_STRU           *pstFsmMsgBuffAddr   = NAS_LMM_NULL_PTR;
    XCC_LMM_CALL_START_NTF_STRU        *pstXccCallStartNtf  = NAS_LMM_NULL_PTR;


    NAS_LMM_PUBM_LOG_INFO("NAS_EMM_PreProcXccCallStartNtf Enter.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_PreProcXccCallStartNtf_ENUM,LNAS_ENTRY);

    pstXccCallStartNtf = (XCC_LMM_CALL_START_NTF_STRU*)pMsg;

    if(VOS_FALSE == NAS_EMM_IsSupportSrlte())
    {
        NAS_LMM_PUBM_LOG_INFO("NAS_EMM_PreProcXccCallStartNtf not support SRLTE.");
        TLPS_PRINT2LAYER_INFO(NAS_EMM_PreProcXccCallStartNtf_ENUM,LNAS_FUNCTION_LABEL1);

        NAS_EMM_SER_SaveXccCallFlg(NAS_EMM_XCC_CALL_FLAG_VALID);

        NAS_EMM_ERABM_SendCallStartInd();
        NAS_EMM_XCC_SendEsrEndInd(LMM_XCC_ESR_RSLT_NOT_SEND);
        return NAS_LMM_MSG_HANDLED;
    }

    /* 所有收到消息, pMmPidMsg中的BYTE4 清为0 */
    ulMsgType = ID_XCC_LMM_CALL_END_NTF  & NAS_LMM_MSGID_DPID_POS;

    pstFsmMsgBuffAddr                   = NAS_LMM_GetFsmBufAddr( NAS_LMM_PARALLEL_FSM_EMM );

    if(NAS_LMM_NULL_PTR != pstFsmMsgBuffAddr)
    {
        /*如果CALL END消息存在，移去该消息 */
        NAS_LMM_RemoveMsgFromFsmBufferQue( pstFsmMsgBuffAddr, ulMsgType, NAS_LMM_STORE_HIGH_PRIO_MSG,UEPS_PID_XCC);
    }

    NAS_EMM_SER_SaveXccCallOpid(pstXccCallStartNtf->usOpId);
    switch(NAS_EMM_CUR_MAIN_STAT)
    {
        case    EMM_MS_SER_INIT:
                return NAS_LMM_MSG_DISCARD;

        case    EMM_MS_REG:
                return NAS_EMM_MsRegPreProcXccCallStartNtfMsg();

        default:
                NAS_EMM_SER_SaveXccCallFlg(NAS_EMM_XCC_CALL_FLAG_VALID);

                NAS_EMM_ERABM_SendCallStartInd();
                NAS_EMM_XCC_SendEsrEndInd(LMM_XCC_ESR_RSLT_NOT_SEND);
                break;
    }

    return NAS_LMM_MSG_HANDLED;
}



VOS_UINT32 NAS_EMM_MsRegSsNormalOrAttemptUpdateMmMsgXccCallStartNtf
(
    VOS_UINT32                          ulMsgId,
    VOS_VOID                           *pMsgStru
)
{
    XCC_LMM_CALL_START_NTF_STRU        *pstCallStartNtf = VOS_NULL_PTR;
    VOS_UINT32                          ulTimerLen      = NAS_EMM_NULL;


    (VOS_VOID)ulMsgId;

    NAS_EMM_SER_LOG_INFO("NAS_EMM_MsRegSsNormalOrAttemptUpdateMmMsgXccCallStartNtf entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegSsNormalOrAttemptUpdateMmMsgXccCallStartNtf_ENUM,LNAS_ENTRY);

    pstCallStartNtf = (XCC_LMM_CALL_START_NTF_STRU *)pMsgStru;

    if((NAS_EMM_CONN_RELEASING == NAS_EMM_GetConnState())
     ||(NAS_EMM_CONN_WAIT_SYS_INFO == NAS_EMM_GetConnState())
     ||(NAS_LMM_TIMER_RUNNING == NAS_LMM_IsStaTimerRunning(TI_NAS_EMM_STATE_T3440)))
    {
        NAS_EMM_SER_LOG_INFO("NAS_EMM_MsRegSsNormalOrAttemptUpdateMmMsgXccCallStartNtf not connect or idle .");
        TLPS_PRINT2LAYER_INFO(NAS_EMM_MsRegSsNormalOrAttemptUpdateMmMsgXccCallStartNtf_ENUM,LNAS_FUNCTION_LABEL1);

        NAS_EMM_SER_SaveXccCallFlg(NAS_EMM_XCC_CALL_FLAG_VALID);

        NAS_EMM_ERABM_SendCallStartInd();
        NAS_EMM_XCC_SendEsrEndInd(LMM_XCC_ESR_RSLT_NOT_SEND);
        return NAS_LMM_MSG_HANDLED;
    }

    /*设置SER触发原因值*/
    if (XCC_LMM_CALL_TYPE_MO == pstCallStartNtf->enCallType)
    {
        NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_MO_CSFB_REQ);
    }

    else
    {
        NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_MT_CSFB_REQ);
    }

    /* 设置UE接受CSFB */
    NAS_EMM_SER_SaveEmmSerCsfbRsp(NAS_EMM_CSFB_RSP_ACCEPTED_BY_UE);

    ulTimerLen = NAS_LMM_GetT3417extTimerLen(NAS_EMM_ESR_CAUSE_TYPE_CL_CALL);

    NAS_LMM_ModifyStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417_EXT,ulTimerLen);

    /*启动定时器 */
    NAS_LMM_StartStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417_EXT);

    /*转换EMM状态机MS_SER_INIT+SS_SER_WAIT_CN_CNF*/
    NAS_EMM_TAUSER_FSMTranState(EMM_MS_SER_INIT, EMM_SS_SER_WAIT_CN_SER_CNF, TI_NAS_EMM_STATE_SERVICE_T3417_EXT);

    NAS_EMM_SndLrrcLmmCsfbNotify(LRRC_LNAS_CSFB_STATUS_INIT);

    /*组合并发送MRRC_DATA_REQ(SERVICE_REQ)*/
    NAS_EMM_SER_SendMrrcDataReq_ExtendedServiceReq();
    return NAS_LMM_MSG_HANDLED;
}


VOS_UINT32 NAS_EMM_MsSerInitMsgXccCallStartNtf
(
    VOS_UINT32                          ulMsgId,
    VOS_VOID                           *pMsgStru
)
{
    XCC_LMM_CALL_START_NTF_STRU        *pstCallStartNtf = VOS_NULL_PTR;
    VOS_UINT32                          ulTimerLen      = NAS_EMM_NULL;

    (VOS_VOID)ulMsgId;

    NAS_EMM_SER_LOG_INFO("NAS_EMM_MsSerInitMsgXccCallStartNtf entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_MsSerInitMsgXccCallStartNtf_ENUM,LNAS_ENTRY);

    pstCallStartNtf = (XCC_LMM_CALL_START_NTF_STRU *)pMsgStru;

    /* 当前SRLTE ESR过程中不处理(收到第一条call start后收到call end(缓存)，马上收到第二条call start) */
    if((VOS_TRUE == NAS_EMM_IsSupportSrlte())
        &&(VOS_TRUE == NAS_EMM_SER_IsCsfbProcedure()))
    {
        if (XCC_LMM_CALL_TYPE_MO == pstCallStartNtf->enCallType)
        {
            NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_MO_CSFB_REQ);
        }
        else
        {
            NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_MT_CSFB_REQ);
        }
        return NAS_LMM_MSG_HANDLED;
    }

    if(NAS_EMM_CONN_ESTING == NAS_EMM_GetConnState())
    {
        NAS_EMM_SER_SaveXccCallFlg(NAS_EMM_XCC_CALL_FLAG_VALID);

        NAS_EMM_ERABM_SendCallStartInd();
        NAS_EMM_XCC_SendEsrEndInd(LMM_XCC_ESR_RSLT_NOT_SEND);
        return NAS_LMM_MSG_HANDLED;
    }

    /*设置SER触发原因值*/
    if (XCC_LMM_CALL_TYPE_MO == pstCallStartNtf->enCallType)
    {
        NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_MO_CSFB_REQ);
    }
    else
    {
        NAS_EMM_SER_SaveEmmSERStartCause(NAS_EMM_SER_START_CAUSE_MT_CSFB_REQ);
    }

    /* 设置UE接受CSFB */
    NAS_EMM_SER_SaveEmmSerCsfbRsp(NAS_EMM_CSFB_RSP_ACCEPTED_BY_UE);

    ulTimerLen = NAS_LMM_GetT3417extTimerLen(NAS_EMM_ESR_CAUSE_TYPE_CL_CALL);

    NAS_LMM_ModifyStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417_EXT,ulTimerLen);

    /*启动定时器 */
    NAS_LMM_StartStateTimer(TI_NAS_EMM_STATE_SERVICE_T3417_EXT);

    /*转换EMM状态机MS_SER_INIT+SS_SER_WAIT_CN_CNF*/
    NAS_EMM_TAUSER_FSMTranState(EMM_MS_SER_INIT, EMM_SS_SER_WAIT_CN_SER_CNF, TI_NAS_EMM_STATE_SERVICE_T3417_EXT);

    NAS_EMM_SndLrrcLmmCsfbNotify(LRRC_LNAS_CSFB_STATUS_INIT);

    /*组合并发送MRRC_DATA_REQ(SERVICE_REQ)*/
    NAS_EMM_SER_SendMrrcDataReq_ExtendedServiceReq();
    return NAS_LMM_MSG_HANDLED;
}


VOS_VOID NAS_EMM_XCC_SendEsrEndInd
(
    LMM_XCC_ESR_RSLT_ENUM_UINT8 enEsrRslt
)
{
    LMM_XCC_ESR_END_IND_STRU           *pstXccEsrEndInd = NAS_EMM_NULL_PTR;

    NAS_EMM_SER_LOG_INFO("NAS_EMM_XCC_SendEsrEndInd entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_XCC_SendEsrEndInd_ENUM,LNAS_ENTRY);

    /* 申请DOPRA消息 */
    pstXccEsrEndInd = (VOS_VOID *) NAS_LMM_ALLOC_MSG(sizeof(LMM_XCC_ESR_END_IND_STRU));
    if (NAS_LMM_NULL_PTR == pstXccEsrEndInd)
    {
        return;
    }

    /* 清空 */
    NAS_LMM_MEM_SET_S(  pstXccEsrEndInd,
        sizeof(LMM_XCC_ESR_END_IND_STRU),
        0,
        sizeof(LMM_XCC_ESR_END_IND_STRU));

    /* 打包VOS消息头 */
    NAS_EMM_SET_XCC_MSG_HEADER((pstXccEsrEndInd),
        NAS_EMM_GET_MSG_LENGTH_NO_HEADER(LMM_XCC_ESR_END_IND_STRU));

    /* 填充消息ID */
    pstXccEsrEndInd->enMsgId            = ID_LMM_XCC_ESR_END_IND;
    pstXccEsrEndInd->usOpId             = NAS_EMM_SER_GetXccCallOpid();
    pstXccEsrEndInd->enEsrRslt          = enEsrRslt;

    /* 发送DOPRA消息 */
    NAS_LMM_SEND_MSG(pstXccEsrEndInd);
}


VOS_VOID NAS_EMM_ERABM_SendCallStartInd(VOS_VOID)
{
    EMM_ERABM_CALL_START_NTF_STRU       *pstErabmCallStartNtf = NAS_EMM_NULL_PTR;

    NAS_EMM_SER_LOG_INFO("NAS_EMM_ERABM_SendCallStartInd entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_ERABM_SendCallStartInd_ENUM,LNAS_ENTRY);

    /* 申请DOPRA消息 */
    pstErabmCallStartNtf = (VOS_VOID *) NAS_LMM_ALLOC_MSG(sizeof(EMM_ERABM_CALL_START_NTF_STRU));
    if (NAS_LMM_NULL_PTR == pstErabmCallStartNtf)
    {
        return ;
    }

    /* 清空 */
    NAS_LMM_MEM_SET_S(  pstErabmCallStartNtf,
                        sizeof(EMM_ERABM_CALL_START_NTF_STRU),
                        0,
                        sizeof(EMM_ERABM_CALL_START_NTF_STRU));

    /* 打包VOS消息头 */
    EMM_PUBU_COMP_ERABM_MSG_HEADER((pstErabmCallStartNtf),
            NAS_EMM_GET_MSG_LENGTH_NO_HEADER(EMM_ERABM_CALL_START_NTF_STRU));

    /* 填充消息ID */
    pstErabmCallStartNtf->enMsgId            = ID_EMM_ERABM_CALL_START_NTF;

    /* 发送DOPRA消息 */
    NAS_LMM_SEND_MSG(pstErabmCallStartNtf);

}


VOS_VOID NAS_EMM_ERABM_SendCallEndInd(VOS_VOID)
{
    EMM_ERABM_CALL_END_NTF_STRU           *pstErabmCallEndNtf = NAS_EMM_NULL_PTR;

    NAS_EMM_SER_LOG_INFO("NAS_EMM_ERABM_SendCallEndInd entered.");
    TLPS_PRINT2LAYER_INFO(NAS_EMM_ERABM_SendCallEndInd_ENUM,LNAS_ENTRY);

    /* 申请DOPRA消息 */
    pstErabmCallEndNtf = (VOS_VOID *) NAS_LMM_ALLOC_MSG(sizeof(EMM_ERABM_CALL_END_NTF_STRU));
    if (NAS_LMM_NULL_PTR == pstErabmCallEndNtf)
    {
        return ;
    }

    /* 清空 */
    NAS_LMM_MEM_SET_S(  pstErabmCallEndNtf,
                        sizeof(EMM_ERABM_CALL_END_NTF_STRU),
                        0,
                        sizeof(EMM_ERABM_CALL_END_NTF_STRU));

    /* 打包VOS消息头 */
    EMM_PUBU_COMP_ERABM_MSG_HEADER((pstErabmCallEndNtf),
            NAS_EMM_GET_MSG_LENGTH_NO_HEADER(EMM_ERABM_CALL_END_NTF_STRU));

    /* 填充消息ID */
    pstErabmCallEndNtf->enMsgId            = ID_EMM_ERABM_CALL_END_NTF;

    /* 发送DOPRA消息 */
    NAS_LMM_SEND_MSG(pstErabmCallEndNtf);
}
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif




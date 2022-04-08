/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2022 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "nr-mac-scheduler-ue-info-rr.h"

namespace ns3
{

/**
 * \brief Table of Priorities per QCI, as per TS23.501 Table 5.7.4-1
 */
static const std::vector<double> prioritiesPerQci = {
    0, 20, 40, 30, 50, 10, 60, 70, 80, 90, 0, // QCI 0 to 10
    0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0,  0, 0, 0,  0,  0, 0,  0,  0,  0,  0, 0,
    0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0,  0, 0, 0,  0,  0, 0,  0,  0,  0,  0, 0,
    0, 0,  0,  0,  0,  0,  0,  0, // QCI 11 to 64
    7, 20, 15, 0,  5,  55, 56, 56, 56, 56, 0, 56, 0, 0, 65, 68, 0, 19, 22, 24, 21, 18 // QCI 65 to
                                                                                      // 86
};

/**
 * \ingroup scheduler
 * \brief UE representation for a QoS-based scheduler
 *
 * The representation stores the current throughput, the average throughput,
 * and the last average throughput, as well as providing comparison functions
 * to sort the UEs in case of a QoS scheduler, according to its QCI and priority.
 *
 * \see CompareUeWeightsDl
 * \see CompareUeWeightsUl
 */
class NrMacSchedulerUeInfoQos : public NrMacSchedulerUeInfo
{
  public:
    /**
     * \brief NrMacSchedulerUeInfoQos constructor
     * \param rnti RNTI of the UE
     * \param beamConfId BeamConfId of the UE
     * \param fn A function that tells how many RB per RBG
     */
    NrMacSchedulerUeInfoQos(float alpha,
                            uint16_t rnti,
                            BeamConfId beamConfId,
                            const GetRbPerRbgFn& fn)
        : NrMacSchedulerUeInfo(rnti, beamConfId, fn),
          m_alpha(alpha)
    {
    }

    /**
     * \brief Reset DL QoS scheduler info
     *
     * Set the last average throughput to the current average throughput,
     * and zeroes the average throughput as well as the current throughput.
     *
     * It calls also NrMacSchedulerUeInfoQos::ResetDlSchedInfo.
     */
    void ResetDlSchedInfo() override
    {
        m_lastAvgTputDl = m_avgTputDl;
        m_avgTputDl = 0.0;
        m_currTputDl = 0.0;
        m_potentialTputDl = 0.0;
        NrMacSchedulerUeInfo::ResetDlSchedInfo();
    }

    /**
     * \brief Reset UL QoS scheduler info
     *
     * Set the last average throughput to the current average throughput,
     * and zeroes the average throughput as well as the current throughput.
     *
     * It also calls NrMacSchedulerUeInfoQos::ResetUlSchedInfo.
     */
    void ResetUlSchedInfo() override
    {
        m_lastAvgTputUl = m_avgTputUl;
        m_avgTputUl = 0.0;
        m_currTputUl = 0.0;
        m_potentialTputUl = 0.0;
        NrMacSchedulerUeInfo::ResetUlSchedInfo();
    }

    /**
     * \brief Reset the DL avg Th to the last value
     */
    void ResetDlMetric() override
    {
        NrMacSchedulerUeInfo::ResetDlMetric();
        m_avgTputDl = m_lastAvgTputDl;
    }

    /**
     * \brief Reset the UL avg Th to the last value
     */
    void ResetUlMetric() override
    {
        NrMacSchedulerUeInfo::ResetUlMetric();
        m_avgTputUl = m_lastAvgTputUl;
    }

    /**
     * \brief Update the QoS metric for downlink
     * \param totAssigned the resources assigned
     * \param timeWindow the time window
     * \param amc a pointer to the AMC
     *
     * Updates m_currTputDl and m_avgTputDl by keeping in consideration
     * the assigned resources (in form of TBS) and the time window.
     * It gets the tbSise by calling NrMacSchedulerUeInfo::UpdateDlMetric.
     */
    void UpdateDlQosMetric(const NrMacSchedulerNs3::FTResources& totAssigned,
                           double timeWindow,
                           const Ptr<const NrAmc>& amc);

    /**
     * \brief Update the QoS metric for uplink
     * \param totAssigned the resources assigned
     * \param timeWindow the time window
     * \param amc a pointer to the AMC
     *
     * Updates m_currTputUl and m_avgTputUl by keeping in consideration
     * the assigned resources (in form of TBS) and the time window.
     * It gets the tbSise by calling NrMacSchedulerUeInfo::UpdateUlMetric.
     */
    void UpdateUlQosMetric(const NrMacSchedulerNs3::FTResources& totAssigned,
                           double timeWindow,
                           const Ptr<const NrAmc>& amc);

    /**
     * \brief Calculate the Potential throughput for downlink
     * \param assignableInIteration resources assignable
     * \param amc a pointer to the AMC
     */
    void CalculatePotentialTPutDl(const NrMacSchedulerNs3::FTResources& assignableInIteration,
                                  const Ptr<const NrAmc>& amc);

    /**
     * \brief Calculate the Potential throughput for uplink
     * \param assignableInIteration resources assignable
     * \param amc a pointer to the AMC
     */
    void CalculatePotentialTPutUl(const NrMacSchedulerNs3::FTResources& assignableInIteration,
                                  const Ptr<const NrAmc>& amc);

    /**
     * \brief comparison function object (i.e. an object that satisfies the
     * requirements of Compare) which returns ​true if the first argument is less
     * than (i.e. is ordered before) the second.
     * \param lue Left UE
     * \param rue Right UE
     * \return true if the QoS metric of the left UE is higher than the right UE
     *
     * The QoS metric is calculated as following:
     *
     * \f$ qosMetric_{i} = P * std::pow(potentialTPut_{i}, alpha) / std::max (1E-9, m_avgTput_{i})
     * \f$
     *
     * Alpha is a fairness metric. P is the priority associated to the QCI.
     * Please note that the throughput is calculated in bit/symbol.
     */
    static bool CompareUeWeightsDl(const NrMacSchedulerNs3::UePtrAndBufferReq& lue,
                                   const NrMacSchedulerNs3::UePtrAndBufferReq& rue)
    {
        auto luePtr = dynamic_cast<NrMacSchedulerUeInfoQos*>(lue.first.get());
        auto ruePtr = dynamic_cast<NrMacSchedulerUeInfoQos*>(rue.first.get());

        uint8_t leftUeMinQci = CalculateDlMinQci(lue);
        uint8_t rightUeMinQci = CalculateDlMinQci(rue);

        double leftP = prioritiesPerQci.at(leftUeMinQci);
        double rightP = prioritiesPerQci.at(rightUeMinQci);
        NS_ABORT_IF(leftP == 0);
        NS_ABORT_IF(rightP == 0);

        double lQoSMetric = leftP * std::pow(luePtr->m_potentialTputDl, luePtr->m_alpha) /
                            std::max(1E-9, luePtr->m_avgTputDl);
        double rQoSMetric = rightP * std::pow(ruePtr->m_potentialTputDl, ruePtr->m_alpha) /
                            std::max(1E-9, ruePtr->m_avgTputDl);

        return (lQoSMetric > rQoSMetric);
    }

    /**
     * \brief comparison function object (i.e. an object that satisfies the
     * requirements of Compare) which returns ​true if the first argument is less
     * than (i.e. is ordered before) the second.
     * \param lue Left UE
     * \param rue Right UE
     * \return true if the QoS metric of the left UE is higher than the right UE
     *
     * The QoS metric is calculated as following:
     *
     * \f$ qosMetric_{i} = P * std::pow(potentialTPut_{i}, alpha) / std::max (1E-9, m_avgTput_{i})
     * \f$
     *
     * Alpha is a fairness metric. P is the priority associated to the QCI.
     * Please note that the throughput is calculated in bit/symbol.
     */
    static bool CompareUeWeightsUl(const NrMacSchedulerNs3::UePtrAndBufferReq& lue,
                                   const NrMacSchedulerNs3::UePtrAndBufferReq& rue)
    {
        auto luePtr = dynamic_cast<NrMacSchedulerUeInfoQos*>(lue.first.get());
        auto ruePtr = dynamic_cast<NrMacSchedulerUeInfoQos*>(rue.first.get());

        uint8_t leftUeMinQci = CalculateUlMinQci(lue);
        uint8_t rightUeMinQci = CalculateUlMinQci(rue);

        double leftP = prioritiesPerQci.at(leftUeMinQci);
        double rightP = prioritiesPerQci.at(rightUeMinQci);
        NS_ABORT_IF(leftP == 0);
        NS_ABORT_IF(rightP == 0);

        double lQoSMetric = leftP * std::pow(luePtr->m_potentialTputUl, luePtr->m_alpha) /
                            std::max(1E-9, luePtr->m_avgTputUl);
        double rQoSMetric = rightP * std::pow(ruePtr->m_potentialTputUl, ruePtr->m_alpha) /
                            std::max(1E-9, ruePtr->m_avgTputUl);

        return (lQoSMetric > rQoSMetric);
    }

    /**
     * \brief This function calculates the min QCI.
     * \param lue Left UE
     * \param rue Right UE
     * \return true if the QCI of lue is less than the QCI of rue
     *
     * The ordering is made by considering the minimum QCI among all the QCIs of
     * all the LCs set for this UE.
     * A UE that has a QCI = 1 will always be the first (i.e., has an higher priority)
     * in a QoS scheduler.
     */
    static uint8_t CalculateDlMinQci(const NrMacSchedulerNs3::UePtrAndBufferReq& ue)
    {
        uint8_t ueMinQci = 100;

        for (const auto& ueLcg : ue.first->m_dlLCG)
        {
            std::vector<uint8_t> ueActiveLCs = ueLcg.second->GetActiveLCIds();

            for (const auto lcId : ueActiveLCs)
            {
                std::unique_ptr<NrMacSchedulerLC>& LCPtr = ueLcg.second->GetLC(lcId);

                if (ueMinQci > LCPtr->m_qci)
                {
                    ueMinQci = LCPtr->m_qci;
                }
            }
        }
        return ueMinQci;
    }

    static uint8_t CalculateUlMinQci(const NrMacSchedulerNs3::UePtrAndBufferReq& ue)
    {
        uint8_t ueMinQci = 100;

        for (const auto& ueLcg : ue.first->m_ulLCG)
        {
            std::vector<uint8_t> ueActiveLCs = ueLcg.second->GetActiveLCIds();

            for (const auto lcId : ueActiveLCs)
            {
                std::unique_ptr<NrMacSchedulerLC>& LCPtr = ueLcg.second->GetLC(lcId);

                if (ueMinQci > LCPtr->m_qci)
                {
                    ueMinQci = LCPtr->m_qci;
                }
            }
        }
        return ueMinQci;
    }

    double m_currTputDl{0.0};      //!< Current slot throughput in downlink
    double m_avgTputDl{0.0};       //!< Average throughput in downlink during all the slots
    double m_lastAvgTputDl{0.0};   //!< Last average throughput in downlink
    double m_potentialTputDl{0.0}; //!< Potential throughput in downlink in one assignable resource
                                   //!< (can be a symbol or a RBG)
    float m_alpha{0.0};            //!< PF fairness metric

    double m_currTputUl{0.0};      //!< Current slot throughput in uplink
    double m_avgTputUl{0.0};       //!< Average throughput in uplink during all the slots
    double m_lastAvgTputUl{0.0};   //!< Last average throughput in uplink
    double m_potentialTputUl{0.0}; //!< Potential throughput in uplink in one assignable resource
                                   //!< (can be a symbol or a RBG)
};

} // namespace ns3
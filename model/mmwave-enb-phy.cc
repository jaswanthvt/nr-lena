/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *   Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation;
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define NS_LOG_APPEND_CONTEXT                                            \
  do                                                                     \
    {                                                                    \
      if (m_phyMacConfig)                                                \
        {                                                                \
          std::clog << " [ CellId " << m_cellId << ", ccId "             \
                    << +m_phyMacConfig->GetCcId () << "] ";              \
        }                                                                \
    }                                                                    \
  while (false);

#include <ns3/log.h>
#include <ns3/lte-radio-bearer-tag.h>
#include <ns3/node.h>
#include <algorithm>
#include <functional>

#include "mmwave-enb-phy.h"
#include "mmwave-ue-phy.h"
#include "mmwave-net-device.h"
#include "mmwave-ue-net-device.h"
#include "mmwave-radio-bearer-tag.h"
#include "nr-ch-access-manager.h"

#include <ns3/node-list.h>
#include <ns3/node.h>
#include <ns3/pointer.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MmWaveEnbPhy");

NS_OBJECT_ENSURE_REGISTERED (MmWaveEnbPhy);

MmWaveEnbPhy::MmWaveEnbPhy ()
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("This constructor should not be called");
}

MmWaveEnbPhy::MmWaveEnbPhy (Ptr<MmWaveSpectrumPhy> dlPhy, Ptr<MmWaveSpectrumPhy> ulPhy,
                            const Ptr<Node> &n)
  : MmWavePhy (dlPhy, ulPhy)
{
  m_enbCphySapProvider = new MemberLteEnbCphySapProvider<MmWaveEnbPhy> (this);

  Simulator::ScheduleWithContext (n->GetId (), MilliSeconds (0),
                                  &MmWaveEnbPhy::StartSlot, this, 0, 0, 0);
}

MmWaveEnbPhy::~MmWaveEnbPhy ()
{

}

TypeId
MmWaveEnbPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MmWaveEnbPhy")
    .SetParent<MmWavePhy> ()
    .AddConstructor<MmWaveEnbPhy> ()
    .AddAttribute ("TxPower",
                   "Transmission power in dBm",
                   DoubleValue (4.0),
                   MakeDoubleAccessor (&MmWaveEnbPhy::SetTxPower,
                                       &MmWaveEnbPhy::GetTxPower),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("NoiseFigure",
                   "Loss (dB) in the Signal-to-Noise-Ratio due to non-idealities in the receiver."
                   " According to Wikipedia (http://en.wikipedia.org/wiki/Noise_figure), this is "
                   "\"the difference in decibels (dB) between"
                   " the noise output of the actual receiver to the noise output of an "
                   " ideal receiver with the same overall gain and bandwidth when the receivers "
                   " are connected to sources at the standard noise temperature T0.\" "
                   "In this model, we consider T0 = 290K.",
                   DoubleValue (5.0),
                   MakeDoubleAccessor (&MmWaveEnbPhy::m_noiseFigure),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("DlSpectrumPhy",
                   "The downlink MmWaveSpectrumPhy associated to this MmWavePhy",
                   TypeId::ATTR_GET,
                   PointerValue (),
                   MakePointerAccessor (&MmWaveEnbPhy::GetDlSpectrumPhy),
                   MakePointerChecker <MmWaveSpectrumPhy> ())
    .AddTraceSource ("UlSinrTrace",
                     "UL SINR statistics.",
                     MakeTraceSourceAccessor (&MmWaveEnbPhy::m_ulSinrTrace),
                     "ns3::UlSinr::TracedCallback")
    .AddTraceSource ("EnbPhyRxedCtrlMsgsTrace",
                     "Enb PHY Rxed Control Messages Traces.",
                     MakeTraceSourceAccessor (&MmWaveEnbPhy::m_phyRxedCtrlMsgsTrace),
                     "ns3::MmWavePhyRxTrace::RxedEnbPhyCtrlMsgsTracedCallback")
    .AddTraceSource ("EnbPhyTxedCtrlMsgsTrace",
                     "Enb PHY Txed Control Messages Traces.",
                     MakeTraceSourceAccessor (&MmWaveEnbPhy::m_phyTxedCtrlMsgsTrace),
                     "ns3::MmWavePhyRxTrace::TxedEnbPhyCtrlMsgsTracedCallback")
    .AddAttribute ("MmWavePhyMacCommon",
                   "The associated MmWavePhyMacCommon",
                   PointerValue (),
                   MakePointerAccessor (&MmWaveEnbPhy::m_phyMacConfig),
                   MakePointerChecker<MmWaveEnbPhy> ())
    .AddAttribute ("AntennaArrayType",
                   "AntennaArray of this enb phy. There are two types of antenna array available: "
                   "a) AntennaArrayModel which is using isotropic antenna elements, and "
                   "b) AntennaArray3gppModel which is using directional 3gpp antenna elements."
                   "Another important parameters to specify is the number of antenna elements by "
                   "dimension.",
                   TypeIdValue(ns3::AntennaArrayModel::GetTypeId()),
                   MakeTypeIdAccessor (&MmWavePhy::SetAntennaArrayType,
                                       &MmWavePhy::GetAntennaArrayType),
                   MakeTypeIdChecker())
    .AddAttribute ("AntennaNumDim1",
                   "Size of the first dimension of the antenna sector/panel expressed in number of antenna elements",
                   UintegerValue (4),
                   MakeUintegerAccessor (&MmWavePhy::SetAntennaNumDim1,
                                         &MmWavePhy::GetAntennaNumDim1),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("AntennaNumDim2",
                   "Size of the second dimension of the antenna sector/panel expressed in number of antenna elements",
                   UintegerValue (8),
                   MakeUintegerAccessor (&MmWavePhy::SetAntennaNumDim2,
                                         &MmWavePhy::GetAntennaNumDim2),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("BeamformingPeriodicity",
                   "Interval between beamforming phases",
                   TimeValue (MilliSeconds (100)),
                   MakeTimeAccessor (&MmWaveEnbPhy::m_beamformingPeriodicity),
                   MakeTimeChecker())
    ;
  return tid;

}

void
MmWaveEnbPhy::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);

  m_downlinkSpectrumPhy->SetNoisePowerSpectralDensity (GetNoisePowerSpectralDensity());

  MmWavePhy::InstallAntenna();
  NS_ASSERT_MSG (GetAntennaArray(), "Error in initialization of the AntennaModel object");
  Ptr<AntennaArray3gppModel> antenna3gpp = DynamicCast<AntennaArray3gppModel> (GetAntennaArray());
  if (antenna3gpp)
    {
      antenna3gpp->SetIsUe(false);
    }

  m_downlinkSpectrumPhy->SetAntenna (GetAntennaArray());
  m_uplinkSpectrumPhy->SetAntenna (GetAntennaArray());

  NS_LOG_INFO ("eNb antenna array initialised:" << static_cast<uint32_t> (GetAntennaArray()->GetAntennaNumDim1()) <<
               ", " << static_cast<uint32_t> (GetAntennaArray()->GetAntennaNumDim2()));

  if (m_beamformingPeriodicity != MilliSeconds (0))
    {
      m_beamformingTimer = Simulator::Schedule (m_beamformingPeriodicity,
                                                &MmWaveEnbPhy::ExpireBeamformingTimer, this);
    }

  MmWavePhy::DoInitialize ();
}

void
MmWaveEnbPhy::DoDispose (void)
{
  delete m_enbCphySapProvider;
  MmWavePhy::DoDispose ();
}

void
MmWaveEnbPhy::SetConfigurationParameters (const Ptr<MmWavePhyMacCommon> &phyMacCommon)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_phyMacConfig == nullptr);
  m_phyMacConfig = phyMacCommon;

  InitializeMessageList ();
}

void
MmWaveEnbPhy::SetEnbCphySapUser (LteEnbCphySapUser* s)
{
  NS_LOG_FUNCTION (this);
  m_enbCphySapUser = s;
}

LteEnbCphySapProvider*
MmWaveEnbPhy::GetEnbCphySapProvider ()
{
  NS_LOG_FUNCTION (this);
  return m_enbCphySapProvider;
}

AntennaArrayModel::BeamId MmWaveEnbPhy::GetBeamId (uint16_t rnti) const
{
  for (uint8_t i = 0; i < m_deviceMap.size (); i++)
    {
      Ptr<MmWaveUeNetDevice> ueDev = DynamicCast < MmWaveUeNetDevice > (m_deviceMap.at (i));
      uint64_t ueRnti = (DynamicCast<MmWaveUePhy>(ueDev->GetPhy (0)))->GetRnti ();

      if (ueRnti == rnti)
        {
          Ptr<AntennaArrayModel> antennaArray = DynamicCast<AntennaArrayModel> (GetDlSpectrumPhy ()->GetRxAntenna ());
          return AntennaArrayModel::GetBeamId (antennaArray->GetBeamformingVector (m_deviceMap.at (i)));
        }
    }
  return AntennaArrayModel::BeamId (std::make_pair (0,0));
}

void
MmWaveEnbPhy::SetCam (const Ptr<NrChAccessManager> &cam)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (cam != nullptr);
  m_cam = cam;
  m_cam->SetAccessGrantedCallback (std::bind (&MmWaveEnbPhy::ChannelAccessGranted, this,
                                              std::placeholders::_1));
}

void
MmWaveEnbPhy::SetTxPower (double pow)
{
  m_txPower = pow;
}
double
MmWaveEnbPhy::GetTxPower () const
{
  return m_txPower;
}

void
MmWaveEnbPhy::SetSubChannels (const std::vector<int> &rbIndexVector)
{
  Ptr<SpectrumValue> txPsd = GetTxPowerSpectralDensity (rbIndexVector);
  NS_ASSERT (txPsd);
  m_downlinkSpectrumPhy->SetTxPowerSpectralDensity (txPsd);
}

Ptr<MmWaveSpectrumPhy> MmWaveEnbPhy::GetDlSpectrumPhy() const
{
  return m_downlinkSpectrumPhy;
}

void
MmWaveEnbPhy::QueueMib ()
{
  NS_LOG_FUNCTION (this);
  LteRrcSap::MasterInformationBlock mib;
  mib.dlBandwidth = 4U;
  mib.systemFrameNumber = 1;
  Ptr<MmWaveMibMessage> mibMsg = Create<MmWaveMibMessage> ();
  mibMsg->SetMib (mib);
  EnqueueCtrlMsgNow (mibMsg);
}

void MmWaveEnbPhy::QueueSib ()
{
  NS_LOG_FUNCTION (this);
  Ptr<MmWaveSib1Message> msg = Create<MmWaveSib1Message> ();
  msg->SetSib1 (m_sib1);
  EnqueueCtrlMsgNow (msg);
}

void
MmWaveEnbPhy::StartSlot (uint16_t frameNum, uint8_t sfNum, uint16_t slotNum)
{
  NS_LOG_FUNCTION (this);

  m_frameNum = frameNum;
  m_subframeNum = sfNum;
  m_slotNum = static_cast<uint8_t> (slotNum);
  m_lastSlotStart = Simulator::Now ();
  m_varTtiNum = 0;

  m_lastSlotStart = Simulator::Now ();
  m_currSlotAllocInfo = RetrieveSlotAllocInfo ();

  const SfnSf currentSlot = SfnSf (m_frameNum, m_subframeNum, m_slotNum, 0);

  NS_ASSERT ((m_currSlotAllocInfo.m_sfnSf.m_frameNum == m_frameNum)
             && (m_currSlotAllocInfo.m_sfnSf.m_subframeNum == m_subframeNum)
             && (m_currSlotAllocInfo.m_sfnSf.m_slotNum == m_slotNum ));

  if (m_currSlotAllocInfo.m_varTtiAllocInfo.size () == 0)
    {
      NS_LOG_INFO ("gNB start  empty slot " << m_currSlotAllocInfo.m_sfnSf <<
                   " scheduling directly the end of the slot");
      Simulator::Schedule (m_phyMacConfig->GetSlotPeriod (), &MmWaveEnbPhy::EndSlot, this);
      return;
    }

  NS_ASSERT_MSG ((m_currSlotAllocInfo.m_sfnSf.m_frameNum == m_frameNum)
                 && (m_currSlotAllocInfo.m_sfnSf.m_subframeNum == m_subframeNum)
                 && (m_currSlotAllocInfo.m_sfnSf.m_slotNum == m_slotNum ),
                 "Current slot " << SfnSf (m_frameNum, m_subframeNum, m_slotNum, 0) <<
                 " but allocation for " << m_currSlotAllocInfo.m_sfnSf);


  if (m_slotNum == 0)
    {
      if (m_subframeNum == 0)   // send MIB at the beginning of each frame
        {
          QueueMib ();
        }
      else if (m_subframeNum == 5)   // send SIB at beginning of second half-frame
        {
          QueueSib ();
        }
    }

  if (m_channelStatus == GRANTED)
    {
      NS_LOG_DEBUG ("Channel granted; asking MAC for SlotIndication for the future and then start the slot");
      m_phySapUser->SlotUlIndication (currentSlot);
      m_phySapUser->SlotDlIndication (currentSlot);

      DoStartSlot ();
    }
  else
    {
      bool hasUlDci = false;
      const SfnSf ulSfn = currentSlot.CalculateUplinkSlot (m_phyMacConfig->GetUlSchedDelay (),
                                                           m_phyMacConfig->GetSlotsPerSubframe (),
                                                           m_phyMacConfig->GetSubframesPerFrame ());
      if (m_phyMacConfig->GetUlSchedDelay () > 0)
        {
          if (SlotAllocInfoExists (ulSfn))
            {
              SlotAllocInfo & ulSlot = PeekSlotAllocInfo (ulSfn);
              hasUlDci = ulSlot.ContainsDataAllocation ();
            }
        }
      if (m_currSlotAllocInfo.ContainsDataAllocation () || ! IsCtrlMsgListEmpty () || hasUlDci)
        {
          // Request the channel access
          if (m_channelStatus == NONE)
            {
              NS_LOG_DEBUG ("Channel not granted, request the channel");
              m_channelStatus = REQUESTED; // This goes always before RequestAccess()
              m_cam->RequestAccess ();
              if (m_channelStatus == GRANTED)
                {
                  // Repetition but we can have a CAM that gives the channel
                  // instantaneously
                  NS_LOG_DEBUG ("Channel granted; asking MAC for SlotIndication for the future and then start the slot");
                  m_phySapUser->SlotUlIndication (SfnSf (m_frameNum, m_subframeNum, m_slotNum, m_varTtiNum));
                  m_phySapUser->SlotDlIndication (SfnSf (m_frameNum, m_subframeNum, m_slotNum, m_varTtiNum));

                  DoStartSlot ();
                  return; // Exit without calling anything else
                }
            }
          // If the channel was not granted, queue back the allocation,
          // without calling the MAC for a new slot
          auto slotAllocCopy = m_currSlotAllocInfo;
          auto newSfnSf = slotAllocCopy.m_sfnSf.IncreaseNoOfSlots(m_phyMacConfig->GetSlotsPerSubframe(),
                                                                  m_phyMacConfig->GetSubframesPerFrame());
          NS_LOG_INFO ("Queueing allocation in front for " << SfnSf (m_frameNum, m_subframeNum, m_slotNum, 0));
          if (m_currSlotAllocInfo.ContainsDataAllocation ())
            {
              NS_LOG_INFO ("Reason: Current slot allocation has data");
            }
          else
            {
              NS_LOG_INFO ("Reason: CTRL message list is not empty");
            }

          PushFrontSlotAllocInfo (newSfnSf, slotAllocCopy);
        }
      else
        {
          // It's an empty slot; ask the MAC for a new one (maybe a new data will arrive..)
          // and just let the current one go away
          NS_LOG_DEBUG ("Channel not granted; but asking MAC for SlotIndication for the future, maybe there will be data");
          m_phySapUser->SlotUlIndication (SfnSf (m_frameNum, m_subframeNum, m_slotNum, m_varTtiNum));
          m_phySapUser->SlotDlIndication (SfnSf (m_frameNum, m_subframeNum, m_slotNum, m_varTtiNum));
        }
      // Just schedule the end of the slot; we do not have the channel
      Simulator::Schedule (m_phyMacConfig->GetSlotPeriod () - NanoSeconds (1), &MmWaveEnbPhy::EndSlot, this);
      NS_LOG_DEBUG ("Schedule " << Simulator::Now () + m_phyMacConfig->GetSlotPeriod() - NanoSeconds (1));
    }

}

void MmWaveEnbPhy::DoStartSlot ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_ctrlMsgs.size () == 0);
  NS_LOG_INFO ("Gnb Start Slot: " << m_currSlotAllocInfo);

  auto currentDci = m_currSlotAllocInfo.m_varTtiAllocInfo[m_varTtiNum].m_dci;
  auto nextVarTtiStart = m_phyMacConfig->GetSymbolPeriod () * currentDci->m_symStart;

  Simulator::Schedule (nextVarTtiStart, &MmWaveEnbPhy::StartVarTti, this);
}


void
MmWaveEnbPhy::StoreRBGAllocation (const std::shared_ptr<DciInfoElementTdma> &dci)
{
  NS_LOG_FUNCTION (this);

  auto itAlloc = m_rbgAllocationPerSym.find (dci->m_symStart);
  if (itAlloc == m_rbgAllocationPerSym.end ())
    {
      itAlloc = m_rbgAllocationPerSym.insert (std::make_pair (dci->m_symStart, dci->m_rbgBitmask)).first;
    }
  else
    {
      auto & existingRBGBitmask = itAlloc->second;
      NS_ASSERT (existingRBGBitmask.size () == m_phyMacConfig->GetBandwidthInRbg ());
      NS_ASSERT (dci->m_rbgBitmask.size () == m_phyMacConfig->GetBandwidthInRbg ());
      for (uint32_t i = 0; i < m_phyMacConfig->GetBandwidthInRbg (); ++i)
        {
          existingRBGBitmask.at (i) = existingRBGBitmask.at (i) | dci->m_rbgBitmask.at (i);
        }
    }
}

void
MmWaveEnbPhy::ExpireBeamformingTimer()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Beamforming timer expired; programming a beamforming");
  m_performBeamforming = true;
  m_beamformingTimer = Simulator::Schedule (m_beamformingPeriodicity,
                                            &MmWaveEnbPhy::ExpireBeamformingTimer, this);
}

std::list <Ptr<MmWaveControlMessage> >
MmWaveEnbPhy::RetrieveMsgsFromDCIs (const SfnSf &sfn)
{
  std::list <Ptr<MmWaveControlMessage> > ctrlMsgs;

  // find all DL DCI elements in the current slot and create the DL RBG bitmask
  uint8_t lastSymbolDl = 0, lastSymbolUl = 0;

  NS_LOG_INFO ("Retrieving DL allocation (and an eventual UL CTRL) for slot " <<
               m_currSlotAllocInfo.m_sfnSf << " from a set of " <<
               m_currSlotAllocInfo.m_varTtiAllocInfo.size () << " allocations");
  for (const auto & dlAlloc : m_currSlotAllocInfo.m_varTtiAllocInfo)
    {
      if (dlAlloc.m_dci->m_type != DciInfoElementTdma::CTRL
          && dlAlloc.m_dci->m_format == DciInfoElementTdma::DL)
        {
          auto & dciElem = dlAlloc.m_dci;
          NS_ASSERT (dciElem->m_format == DciInfoElementTdma::DL);
          NS_ASSERT (dciElem->m_tbSize > 0);
          NS_ASSERT (dciElem->m_symStart >= lastSymbolDl);
          NS_ASSERT_MSG (dciElem->m_symStart + dciElem->m_numSym <= m_phyMacConfig->GetSymbolsPerSlot (),
                         "symStart: " << static_cast<uint32_t> (dciElem->m_symStart) <<
                         " numSym: " << static_cast<uint32_t> (dciElem->m_numSym) <<
                         " symPerSlot: " << static_cast<uint32_t> (m_phyMacConfig->GetSymbolsPerSlot ()));
          lastSymbolDl = dciElem->m_symStart;

          StoreRBGAllocation (dciElem);

          Ptr<MmWaveTdmaDciMessage> dciMsg = Create<MmWaveTdmaDciMessage> (dciElem);
          dciMsg->SetSfnSf (sfn);

          ctrlMsgs.push_back (dciMsg);
          NS_LOG_INFO ("To send, DL DCI for UE " << dciElem->m_rnti);
        }
      else if (dlAlloc.m_dci->m_type == DciInfoElementTdma::CTRL
               && dlAlloc.m_dci->m_format == DciInfoElementTdma::UL)
        {
          Ptr<MmWaveTdmaDciMessage> dciMsg = Create<MmWaveTdmaDciMessage> (dlAlloc.m_dci);
          dciMsg->SetSfnSf (sfn);

          ctrlMsgs.push_back (dciMsg);
          NS_LOG_INFO ("To send, UL CTRL for all UEs");
        }
    }

  // TODO: REDUCE AMOUNT OF DUPLICATE CODE
  // Get all the DCI for UL. We retrieve that DCIs from a future slot
  // if UlSchedDelay > 0, or this slot if == 0.
  const SfnSf ulSfn = sfn.CalculateUplinkSlot (m_phyMacConfig->GetUlSchedDelay (),
                                               m_phyMacConfig->GetSlotsPerSubframe (),
                                               m_phyMacConfig->GetSubframesPerFrame ());
  if (m_phyMacConfig->GetUlSchedDelay () > 0)
    {
      if (SlotAllocInfoExists (ulSfn))
        {
          SlotAllocInfo & ulSlot = PeekSlotAllocInfo (ulSfn);
          NS_LOG_INFO ("Retrieving UL allocation for slot " << ulSlot.m_sfnSf <<
                       " with a total of " << ulSlot.m_varTtiAllocInfo.size () <<
                       " allocations");
          for (const auto & ulAlloc : ulSlot.m_varTtiAllocInfo)
            {
              if (ulAlloc.m_dci->m_type != DciInfoElementTdma::CTRL
                  && ulAlloc.m_dci->m_format == DciInfoElementTdma::UL)
                {
                  auto dciElem = ulAlloc.m_dci;

                  NS_ASSERT (dciElem->m_format == DciInfoElementTdma::UL);
                  NS_ASSERT (dciElem->m_tbSize > 0);
                  NS_ASSERT_MSG (dciElem->m_symStart >= lastSymbolUl,
                                 "symStart: " << static_cast<uint32_t> (dciElem->m_symStart) <<
                                 " lastSymbolUl " << static_cast<uint32_t> (lastSymbolUl));

                  NS_ASSERT (dciElem->m_symStart + dciElem->m_numSym <= m_phyMacConfig->GetSymbolsPerSlot ());
                  lastSymbolUl = dciElem->m_symStart;

                  Ptr<MmWaveTdmaDciMessage> dciMsg = Create<MmWaveTdmaDciMessage> (dciElem);
                  dciMsg->SetSfnSf (sfn);
                  ctrlMsgs.push_back (dciMsg);

                  NS_LOG_INFO ("To send, UL DCI for UE " << dciElem->m_rnti);
                }
            }
        }
    }
  else
    {
      for (const auto & ulAlloc : m_currSlotAllocInfo.m_varTtiAllocInfo)
        {
          if (ulAlloc.m_dci->m_type != DciInfoElementTdma::CTRL
              && ulAlloc.m_dci->m_format == DciInfoElementTdma::UL)
            {
              auto dciElem = ulAlloc.m_dci;

              NS_ASSERT (dciElem->m_format == DciInfoElementTdma::UL);
              NS_ASSERT (dciElem->m_tbSize > 0);
              NS_ASSERT_MSG (dciElem->m_symStart >= lastSymbolUl,
                             "symStart: " << static_cast<uint32_t> (dciElem->m_symStart) <<
                             " lastSymbolUl " << static_cast<uint32_t> (lastSymbolUl));

              NS_ASSERT (dciElem->m_symStart + dciElem->m_numSym <= m_phyMacConfig->GetSymbolsPerSlot ());
              lastSymbolUl = dciElem->m_symStart;

              Ptr<MmWaveTdmaDciMessage> dciMsg = Create<MmWaveTdmaDciMessage> (dciElem);
              dciMsg->SetSfnSf (sfn);
              ctrlMsgs.push_back (dciMsg);

              NS_LOG_INFO ("To send, UL DCI for UE " << dciElem->m_rnti);
            }
        }
    }

  return ctrlMsgs;
}

Time
MmWaveEnbPhy::DlCtrl (const std::shared_ptr<DciInfoElementTdma> &dci)
{
  NS_LOG_FUNCTION (this);

  if (m_performBeamforming)
    {
      m_performBeamforming = false;
      for (const auto & dev : m_deviceMap)
        {
          m_doBeamforming (m_netDevice, dev);
        }
    }

  SfnSf sfn = SfnSf (m_frameNum, m_subframeNum, m_slotNum, m_varTtiNum);

  // Start with a clean RBG allocation bitmask
  m_rbgAllocationPerSym.clear ();

  // create control messages to be transmitted in DL-Control period
  std::list <Ptr<MmWaveControlMessage>> ctrlMsgs = GetControlMessages ();
  ctrlMsgs.merge (RetrieveMsgsFromDCIs (sfn));

  // TX control period
  Time varTtiPeriod = m_phyMacConfig->GetSymbolPeriod () * m_phyMacConfig->GetDlCtrlSymbols ();

  NS_ASSERT(dci->m_numSym == m_phyMacConfig->GetDlCtrlSymbols ());

  if (ctrlMsgs.size () > 0)
    {
      NS_LOG_DEBUG ("ENB TXing DL CTRL with " << ctrlMsgs.size () << " msgs, frame " << m_frameNum <<
                    " subframe " << static_cast<uint32_t> (m_subframeNum) <<
                    " slot " << static_cast<uint32_t> (m_slotNum) <<
                    " symbols "  << static_cast<uint32_t> (dci->m_symStart) <<
                    "-" << static_cast<uint32_t> (dci->m_symStart + dci->m_numSym - 1) <<
                    " start " << Simulator::Now () <<
                    " end " << Simulator::Now () + varTtiPeriod - NanoSeconds (1.0));

      for (auto ctrlIt = ctrlMsgs.begin (); ctrlIt != ctrlMsgs.end (); ++ctrlIt)
        {
          Ptr<MmWaveControlMessage> msg = (*ctrlIt);
          m_phyTxedCtrlMsgsTrace (SfnSf(m_frameNum, m_subframeNum, m_slotNum, dci->m_symStart),
                                  dci->m_rnti, m_phyMacConfig->GetCcId (), msg);
        }

      SendCtrlChannels (&ctrlMsgs, varTtiPeriod - NanoSeconds (1.0)); // -1 ns ensures control ends before data period
    }
  else
    {
      NS_LOG_INFO ("Scheduled time for DL CTRL but no messages to send");
    }

  return varTtiPeriod;
}

Time
MmWaveEnbPhy::UlCtrl(const std::shared_ptr<DciInfoElementTdma> &dci)
{
  NS_LOG_FUNCTION (this);
  Time varTtiPeriod = m_phyMacConfig->GetSymbolPeriod () * m_phyMacConfig->GetUlCtrlSymbols ();

  NS_LOG_DEBUG ("ENB RXng UL CTRL frame " << m_frameNum <<
                " subframe " << static_cast<uint32_t> (m_subframeNum) <<
                " slot " << static_cast<uint32_t> (m_slotNum) <<
                " symbols "  << static_cast<uint32_t> (dci->m_symStart) <<
                "-" << static_cast<uint32_t> (dci->m_symStart + dci->m_numSym - 1) <<
                " start " << Simulator::Now () <<
                " end " << Simulator::Now () + varTtiPeriod);
  return varTtiPeriod;
}

Time
MmWaveEnbPhy::DlData (const VarTtiAllocInfo& varTtiInfo)
{
  NS_LOG_FUNCTION (this);
  Time varTtiPeriod = m_phyMacConfig->GetSymbolPeriod () * varTtiInfo.m_dci->m_numSym;

  Ptr<PacketBurst> pktBurst = GetPacketBurst (SfnSf (m_frameNum, m_subframeNum, m_slotNum, varTtiInfo.m_dci->m_symStart));
  if (pktBurst && pktBurst->GetNPackets () > 0)
    {
      std::list< Ptr<Packet> > pkts = pktBurst->GetPackets ();
      MmWaveMacPduTag macTag;
      pkts.front ()->PeekPacketTag (macTag);
      NS_ASSERT ((macTag.GetSfn ().m_slotNum == m_slotNum) && (macTag.GetSfn ().m_varTtiNum == varTtiInfo.m_dci->m_symStart));
    }
  else
    {
      // sometimes the UE will be scheduled when no data is queued
      // in this case, send an empty PDU
      MmWaveMacPduTag tag (SfnSf (m_frameNum, m_subframeNum, m_slotNum, varTtiInfo.m_dci->m_symStart));
      Ptr<Packet> emptyPdu = Create <Packet> ();
      MmWaveMacPduHeader header;
      MacSubheader subheader (3, 0);    // lcid = 3, size = 0
      header.AddSubheader (subheader);
      emptyPdu->AddHeader (header);
      emptyPdu->AddPacketTag (tag);
      LteRadioBearerTag bearerTag (varTtiInfo.m_dci->m_rnti, 3, 0);
      emptyPdu->AddPacketTag (bearerTag);
      pktBurst = CreateObject<PacketBurst> ();
      pktBurst->AddPacket (emptyPdu);
    }

  NS_LOG_DEBUG ("ENB TXing DL DATA frame " << m_frameNum <<
                " subframe " << static_cast<uint32_t> (m_subframeNum) <<
                " slot " << static_cast<uint32_t> (m_slotNum) <<
                " symbols "  << static_cast<uint32_t> (varTtiInfo.m_dci->m_symStart) <<
                "-" << static_cast<uint32_t> (varTtiInfo.m_dci->m_symStart + varTtiInfo.m_dci->m_numSym - 1) <<
                " start " << Simulator::Now () + NanoSeconds (1) <<
                " end " << Simulator::Now () + varTtiPeriod - NanoSeconds (2.0));

  Simulator::Schedule (NanoSeconds (1.0), &MmWaveEnbPhy::SendDataChannels, this,
                       pktBurst, varTtiPeriod - NanoSeconds (2.0), varTtiInfo);

  return varTtiPeriod;
}

Time
MmWaveEnbPhy::UlData(const std::shared_ptr<DciInfoElementTdma> &dci)
{
  NS_LOG_INFO (this);

  // Assert: we expect TDMA in UL
  NS_ASSERT (dci->m_rbgBitmask.size () == m_phyMacConfig->GetBandwidthInRbg ());
  NS_ASSERT (static_cast<uint32_t> (std::count (dci->m_rbgBitmask.begin (),
                                                dci->m_rbgBitmask.end (), 1U)) == dci->m_rbgBitmask.size ());

  Time varTtiPeriod = m_phyMacConfig->GetSymbolPeriod () * dci->m_numSym;

  m_downlinkSpectrumPhy->AddExpectedTb (dci->m_rnti, dci->m_ndi,
                                        dci->m_tbSize, dci->m_mcs,
                                        FromRBGBitmaskToRBAssignment (dci->m_rbgBitmask),
                                        dci->m_harqProcess, dci->m_rv, false,
                                        dci->m_symStart, dci->m_numSym);

  bool found = false;
  for (uint8_t i = 0; i < m_deviceMap.size (); i++)
    {
      Ptr<MmWaveUeNetDevice> ueDev = DynamicCast < MmWaveUeNetDevice > (m_deviceMap.at (i));
      uint64_t ueRnti = (DynamicCast<MmWaveUePhy>(ueDev->GetPhy (0)))->GetRnti ();
      if (dci->m_rnti == ueRnti)
        {
          Ptr<AntennaArrayModel> antennaArray = DynamicCast<AntennaArrayModel> (GetDlSpectrumPhy ()->GetRxAntenna ());
          antennaArray->ChangeBeamformingVector (m_deviceMap.at (i));
          found = true;
          break;
        }
    }
  NS_ASSERT (found);

  NS_LOG_DEBUG ("ENB RXing UL DATA frame " << m_frameNum <<
                " subframe " << static_cast<uint32_t> (m_subframeNum) <<
                " slot " << static_cast<uint32_t> (m_slotNum) <<
                " symbols "  << static_cast<uint32_t> (dci->m_symStart) <<
                "-" << static_cast<uint32_t> (dci->m_symStart + dci->m_numSym - 1) <<
                " start " << Simulator::Now () <<
                " end " << Simulator::Now () + varTtiPeriod);
  return varTtiPeriod;
}

void
MmWaveEnbPhy::StartVarTti (void)
{
  NS_LOG_FUNCTION (this);

  //assume the control signal is omni
  Ptr<AntennaArrayModel> antennaArray = DynamicCast<AntennaArrayModel> (GetDlSpectrumPhy ()->GetRxAntenna ());
  antennaArray->ChangeToOmniTx ();

  VarTtiAllocInfo & currVarTti = m_currSlotAllocInfo.m_varTtiAllocInfo[m_varTtiNum];
  m_currSymStart = currVarTti.m_dci->m_symStart;
  SfnSf sfn = SfnSf (m_frameNum, m_subframeNum, m_slotNum, m_varTtiNum);
  NS_LOG_INFO ("Starting VarTti on the AIR " << sfn);

  Time varTtiPeriod;

  NS_ASSERT (currVarTti.m_dci->m_type != DciInfoElementTdma::CTRL_DATA);

  if (currVarTti.m_dci->m_type == DciInfoElementTdma::CTRL)
    {
      if (currVarTti.m_dci->m_format == DciInfoElementTdma::DL)
        {
          varTtiPeriod = DlCtrl (currVarTti.m_dci);
        }
      else if (currVarTti.m_dci->m_format == DciInfoElementTdma::UL)
        {
          varTtiPeriod = UlCtrl (currVarTti.m_dci);
        }
    }
  else  if (currVarTti.m_dci->m_type == DciInfoElementTdma::DATA)
    {
      if (currVarTti.m_dci->m_format == DciInfoElementTdma::DL)
        {
          varTtiPeriod = DlData (currVarTti);
        }
      else if (currVarTti.m_dci->m_format == DciInfoElementTdma::UL)
        {
          varTtiPeriod = UlData (currVarTti.m_dci);
        }
    }

  Simulator::Schedule (varTtiPeriod, &MmWaveEnbPhy::EndVarTti, this);
}

void
MmWaveEnbPhy::EndVarTti (void)
{
  NS_LOG_FUNCTION (this << Simulator::Now ().GetSeconds ());
  auto lastDci = m_currSlotAllocInfo.m_varTtiAllocInfo[m_varTtiNum].m_dci;
  NS_LOG_INFO ("DCI started at symbol " << static_cast<uint32_t> (lastDci->m_symStart) <<
               " which lasted for " << static_cast<uint32_t> (lastDci->m_numSym) <<
               " symbols finished");

  Ptr<AntennaArrayModel> antennaArray = DynamicCast<AntennaArrayModel> (GetDlSpectrumPhy ()->GetRxAntenna ());

  antennaArray->ChangeToOmniTx ();

  if (m_varTtiNum == m_currSlotAllocInfo.m_varTtiAllocInfo.size () - 1)
    {
      EndSlot ();
    }
  else
    {
      m_varTtiNum++;
      auto currentDci = m_currSlotAllocInfo.m_varTtiAllocInfo[m_varTtiNum].m_dci;

      if (lastDci->m_symStart == currentDci->m_symStart)
        {
          NS_LOG_INFO ("DCI " << static_cast <uint32_t> (m_varTtiNum) <<
                       " of " << m_currSlotAllocInfo.m_varTtiAllocInfo.size () - 1 <<
                       " for UE " << currentDci->m_rnti << " starts from symbol " <<
                       static_cast<uint32_t> (currentDci->m_symStart) << " ignoring at PHY");
          EndVarTti ();
        }
      else
        {
          auto nextVarTtiStart = m_phyMacConfig->GetSymbolPeriod () * currentDci->m_symStart;

          NS_LOG_INFO ("DCI " << static_cast <uint32_t> (m_varTtiNum) <<
                       " of " << m_currSlotAllocInfo.m_varTtiAllocInfo.size () - 1 <<
                       " for UE " << currentDci->m_rnti << " starts from symbol " <<
                       static_cast<uint32_t> (currentDci->m_symStart) << " scheduling at PHY, at " <<
                       nextVarTtiStart + m_lastSlotStart << " where last slot start = " <<
                       m_lastSlotStart << " nextVarTti " << nextVarTtiStart);

          Simulator::Schedule (nextVarTtiStart + m_lastSlotStart - Simulator::Now (),
                               &MmWaveEnbPhy::StartVarTti, this);
        }
      // Do not put any code here (tail recursion)
    }
  // Do not put any code here (tail recursion)
}

void
MmWaveEnbPhy::EndSlot (void)
{
  NS_LOG_FUNCTION (this << Simulator::Now ().GetSeconds ());

  Time slotStart = m_lastSlotStart + m_phyMacConfig->GetSlotPeriod () - Simulator::Now ();

  NS_ASSERT_MSG (slotStart > MilliSeconds (0),
                 "lastStart=" << m_lastSlotStart + m_phyMacConfig->GetSlotPeriod () <<
                 " now " <<  Simulator::Now () << " slotStart value" << slotStart);

  SfnSf sfnf (m_frameNum, m_subframeNum, m_slotNum, m_varTtiNum);

  SfnSf retVal = sfnf.IncreaseNoOfSlots (m_phyMacConfig->GetSlotsPerSubframe (),
                                         m_phyMacConfig->GetSubframesPerFrame ());

  Simulator::Schedule (slotStart, &MmWaveEnbPhy::StartSlot, this,
                       retVal.m_frameNum, retVal.m_subframeNum,
                       static_cast<uint8_t> (retVal.m_slotNum));
}

void
MmWaveEnbPhy::SendDataChannels (const Ptr<PacketBurst> &pb, const Time &varTtiPeriod,
                                const VarTtiAllocInfo& varTtiInfo)
{
  if (varTtiInfo.m_isOmni)
    {
      Ptr<AntennaArrayModel> antennaArray = DynamicCast<AntennaArrayModel> (GetDlSpectrumPhy ()->GetRxAntenna ());
      antennaArray->ChangeToOmniTx ();
    }
  else
    {   // update beamforming vectors (currently supports 1 user only)
        //std::map<uint16_t, std::vector<unsigned> >::iterator ueRbIt = varTtiInfo.m_ueRbMap.begin();
        //uint16_t rnti = ueRbIt->first;
      bool found = false;
      for (uint8_t i = 0; i < m_deviceMap.size (); i++)
        {
          Ptr<MmWaveUeNetDevice> ueDev = DynamicCast<MmWaveUeNetDevice> (m_deviceMap.at (i));
          uint64_t ueRnti = (DynamicCast<MmWaveUePhy>(ueDev->GetPhy (0)))->GetRnti ();
          //NS_LOG_UNCOND ("Scheduled rnti:"<<rnti <<" ue rnti:"<< ueRnti);
          if (varTtiInfo.m_dci->m_rnti == ueRnti)
            {
              //NS_LOG_UNCOND ("Change Beamforming Vector");
              Ptr<AntennaArrayModel> antennaArray = DynamicCast<AntennaArrayModel> (GetDlSpectrumPhy ()->GetRxAntenna ());
              antennaArray->ChangeBeamformingVector (m_deviceMap.at (i));
              found = true;
              break;
            }
        }
      NS_ABORT_IF (!found);
    }

  // in the map we stored the RBG allocated by the MAC for this symbol.
  // If the transmission last n symbol (n > 1 && n < 12) the SetSubChannels
  // doesn't need to be called again. In fact, SendDataChannels will be
  // invoked only when the symStart changes.
  NS_ASSERT (varTtiInfo.m_dci != nullptr);
  NS_ASSERT (m_rbgAllocationPerSym.find(varTtiInfo.m_dci->m_symStart) != m_rbgAllocationPerSym.end ());
  SetSubChannels (FromRBGBitmaskToRBAssignment (m_rbgAllocationPerSym.at (varTtiInfo.m_dci->m_symStart)));

  std::list<Ptr<MmWaveControlMessage> > ctrlMsgs;
  m_downlinkSpectrumPhy->StartTxDataFrames (pb, ctrlMsgs, varTtiPeriod, varTtiInfo.m_dci->m_symStart);
}

void
MmWaveEnbPhy::SendCtrlChannels (std::list<Ptr<MmWaveControlMessage> > *ctrlMsgs,
                                const Time &varTtiPeriod)
{
  NS_LOG_FUNCTION (this << "Send Ctrl");

  std::vector <int> fullBwRb (m_phyMacConfig->GetBandwidthInRbs ());
  // The first time set the right values for the phy
  for (uint32_t i = 0; i < m_phyMacConfig->GetBandwidthInRbs (); ++i)
    {
      fullBwRb.at (i) = static_cast<int> (i);
    }

  SetSubChannels (fullBwRb);

  m_downlinkSpectrumPhy->StartTxDlControlFrames (*ctrlMsgs, varTtiPeriod);

  ctrlMsgs->clear ();
}

bool
MmWaveEnbPhy::RegisterUe (uint64_t imsi, const Ptr<NetDevice> &ueDevice)
{
  NS_LOG_FUNCTION (this << imsi);
  std::set <uint64_t>::iterator it;
  it = m_ueAttached.find (imsi);

  if (it == m_ueAttached.end ())
    {
      m_ueAttached.insert (imsi);
      m_deviceMap.push_back (ueDevice);
      return (true);
    }
  else
    {
      NS_LOG_ERROR ("Programming error...UE already attached");
      return (false);
    }
}

void
MmWaveEnbPhy::PhyDataPacketReceived (const Ptr<Packet> &p)
{
  Simulator::ScheduleWithContext (m_netDevice->GetNode ()->GetId (),
                                  MicroSeconds (m_phyMacConfig->GetTbDecodeLatency ()),
                                  &MmWaveEnbPhySapUser::ReceivePhyPdu,
                                  m_phySapUser,
                                  p);
  //  m_phySapUser->ReceivePhyPdu(p);
}

void
MmWaveEnbPhy::GenerateDataCqiReport (const SpectrumValue& sinr)
{
  NS_LOG_FUNCTION (this << sinr);

  Values::const_iterator it;
  MmWaveMacSchedSapProvider::SchedUlCqiInfoReqParameters ulcqi;
  ulcqi.m_ulCqi.m_type = UlCqiInfo::PUSCH;
  int i = 0;
  for (it = sinr.ConstValuesBegin (); it != sinr.ConstValuesEnd (); it++)
    {
      //   double sinrdb = 10 * std::log10 ((*it));
      //       NS_LOG_DEBUG ("ULCQI RB " << i << " value " << sinrdb);
      // convert from double to fixed point notaltion Sxxxxxxxxxxx.xxx
      //   int16_t sinrFp = LteFfConverter::double2fpS11dot3 (sinrdb);
      ulcqi.m_ulCqi.m_sinr.push_back (*it);
      i++;
    }

  // here we use the start symbol index of the var tti in place of the var tti index because the absolute UL var tti index is
  // not known to the scheduler when m_allocationMap gets populated
  ulcqi.m_sfnSf = SfnSf (m_frameNum, m_subframeNum, m_slotNum, m_currSymStart);
  SpectrumValue newSinr = sinr;
  m_ulSinrTrace (0, newSinr, newSinr);
  m_phySapUser->UlCqiReport (ulcqi);
}


void
MmWaveEnbPhy::PhyCtrlMessagesReceived (const std::list<Ptr<MmWaveControlMessage> > &msgList)
{
  NS_LOG_FUNCTION (this);

  auto ctrlIt = msgList.begin ();

  while (ctrlIt != msgList.end ())
    {
      Ptr<MmWaveControlMessage> msg = (*ctrlIt);

      if (msg->GetMessageType () == MmWaveControlMessage::DL_CQI)
        {
          NS_LOG_INFO ("received CQI");
          m_phySapUser->ReceiveControlMessage (msg);

          Ptr<MmWaveDlCqiMessage> dlcqi = DynamicCast<MmWaveDlCqiMessage> (msg);
          DlCqiInfo dlcqiLE = dlcqi->GetDlCqi ();
          m_phyRxedCtrlMsgsTrace (SfnSf (m_frameNum, m_subframeNum, m_slotNum, m_varTtiNum),
                                  dlcqiLE.m_rnti, m_phyMacConfig->GetCcId (), msg);
        }
      else if (msg->GetMessageType () == MmWaveControlMessage::BSR)
        {
          NS_LOG_INFO ("received BSR");
          m_phySapUser->ReceiveControlMessage (msg);

          Ptr<MmWaveBsrMessage> bsrmsg = DynamicCast<MmWaveBsrMessage> (msg);
          MacCeElement macCeEl = bsrmsg->GetBsr();
          m_phyRxedCtrlMsgsTrace (SfnSf (m_frameNum, m_subframeNum, m_slotNum, m_varTtiNum),
                                  macCeEl.m_rnti, m_phyMacConfig->GetCcId (), msg);
        }
      else if (msg->GetMessageType () == MmWaveControlMessage::RACH_PREAMBLE)
        {
          NS_LOG_INFO ("received RACH_PREAMBLE");
          NS_ASSERT (m_cellId > 0);

          Ptr<MmWaveRachPreambleMessage> rachPreamble = DynamicCast<MmWaveRachPreambleMessage> (msg);
          m_phySapUser->ReceiveRachPreamble (rachPreamble->GetRapId ());
          m_phyRxedCtrlMsgsTrace (SfnSf (m_frameNum, m_subframeNum, m_slotNum, m_varTtiNum),
                                  0, m_phyMacConfig->GetCcId (), msg);
        }
      else if (msg->GetMessageType () == MmWaveControlMessage::DL_HARQ)
        {

          Ptr<MmWaveDlHarqFeedbackMessage> dlharqMsg = DynamicCast<MmWaveDlHarqFeedbackMessage> (msg);
          DlHarqInfo dlharq = dlharqMsg->GetDlHarqFeedback ();

          NS_LOG_INFO ("cellId:" << m_cellId << Simulator::Now () <<
                       " received DL_HARQ from: " << dlharq.m_rnti);
          // check whether the UE is connected

          if (m_ueAttachedRnti.find (dlharq.m_rnti) != m_ueAttachedRnti.end ())
            {
              m_phySapUser->ReceiveControlMessage (msg);
              m_phyRxedCtrlMsgsTrace (SfnSf (m_frameNum, m_subframeNum, m_slotNum, m_varTtiNum),
                                      dlharq.m_rnti, m_phyMacConfig->GetCcId (), msg);
            }
        }
      else
        {
          m_phySapUser->ReceiveControlMessage (msg);
        }

      ctrlIt++;
    }

}


////////////////////////////////////////////////////////////
/////////                     sap                 /////////
///////////////////////////////////////////////////////////

void
MmWaveEnbPhy::DoSetBandwidth (uint8_t ulBandwidth, uint8_t dlBandwidth)
{
  NS_LOG_FUNCTION (this << +ulBandwidth << +dlBandwidth);
}

void
MmWaveEnbPhy::DoSetEarfcn (uint16_t ulEarfcn, uint16_t dlEarfcn)
{
  NS_LOG_FUNCTION (this << ulEarfcn << dlEarfcn);
}


void
MmWaveEnbPhy::DoAddUe (uint16_t rnti)
{
  NS_UNUSED (rnti);
  NS_LOG_FUNCTION (this << rnti);
  std::set <uint16_t>::iterator it;
  it = m_ueAttachedRnti.find (rnti);
  if (it == m_ueAttachedRnti.end ())
    {
      m_ueAttachedRnti.insert (rnti);
    }
}

void
MmWaveEnbPhy::DoRemoveUe (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << rnti);

  std::set <uint16_t>::iterator it = m_ueAttachedRnti.find (rnti);
  if (it != m_ueAttachedRnti.end ())
    {
      m_ueAttachedRnti.erase (it);
    }
  else
    {
      NS_FATAL_ERROR ("Impossible to remove UE, not attached!");
    }
}

void
MmWaveEnbPhy::DoSetPa (uint16_t rnti, double pa)
{
  NS_LOG_FUNCTION (this << rnti << pa);
}

void
MmWaveEnbPhy::DoSetTransmissionMode (uint16_t  rnti, uint8_t txMode)
{
  NS_LOG_FUNCTION (this << rnti << +txMode);
  // UL supports only SISO MODE
}

void
MmWaveEnbPhy::DoSetSrsConfigurationIndex (uint16_t  rnti, uint16_t srcCi)
{
  NS_LOG_FUNCTION (this << rnti << srcCi);
}

void
MmWaveEnbPhy::DoSetMasterInformationBlock (LteRrcSap::MasterInformationBlock mib)
{
  NS_LOG_FUNCTION (this);
  NS_UNUSED (mib);
}

void
MmWaveEnbPhy::DoSetSystemInformationBlockType1 (LteRrcSap::SystemInformationBlockType1 sib1)
{
  NS_LOG_FUNCTION (this);
  m_sib1 = sib1;
}

int8_t
MmWaveEnbPhy::DoGetReferenceSignalPower () const
{
  NS_LOG_FUNCTION (this);
  return static_cast<int8_t> (m_txPower);
}

void
MmWaveEnbPhy::SetPhySapUser (MmWaveEnbPhySapUser* ptr)
{
  m_phySapUser = ptr;
}

void
MmWaveEnbPhy::ReceiveUlHarqFeedback (const UlHarqInfo &mes)
{
  NS_LOG_FUNCTION (this);
  // forward to scheduler
  if (m_ueAttachedRnti.find (mes.m_rnti) != m_ueAttachedRnti.end ())
    {
      m_phySapUser->UlHarqFeedback (mes);
    }
}

void
MmWaveEnbPhy::SetPerformBeamformingFn(const MmWaveEnbPhy::PerformBeamformingFn &fn)
{
  NS_LOG_FUNCTION (this);
  m_doBeamforming = fn;
}

void
MmWaveEnbPhy::ChannelAccessGranted (const Time &time)
{
  NS_LOG_FUNCTION (this);
  m_channelStatus = GRANTED;

  Time toNextSlot = m_lastSlotStart + m_phyMacConfig->GetSlotPeriod () - Simulator::Now ();
  Time grant = time - toNextSlot;
  int64_t slotGranted = grant.GetNanoSeconds () / m_phyMacConfig->GetSlotPeriod().GetNanoSeconds ();

  NS_LOG_INFO ("Channel access granted for " << time.GetMilliSeconds () <<
               " ms, which corresponds to " << slotGranted << " slot in which each slot is " <<
               m_phyMacConfig->GetSlotPeriod() << " ms. We lost " <<
               toNextSlot.GetMilliSeconds() << " ms. ");
  NS_LOG_DEBUG ("Channel access granted for " << slotGranted << " slot");
  m_channelLostTimer = Simulator::Schedule (m_phyMacConfig->GetSlotPeriod () * slotGranted - NanoSeconds (1),
                                            &MmWaveEnbPhy::ChannelAccessLost, this);
}

void
MmWaveEnbPhy::ChannelAccessLost ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Channel access lost");
  m_channelStatus = NONE;
}

}

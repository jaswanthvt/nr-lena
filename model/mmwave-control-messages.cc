/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
*   Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
*   Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering, New York University
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
*
*   Author: Marco Miozzo <marco.miozzo@cttc.es>
*           Nicola Baldo  <nbaldo@cttc.es>
*
*   Modified by: Marco Mezzavilla < mezzavilla@nyu.edu>
*                         Sourjya Dutta <sdutta@nyu.edu>
*                         Russell Ford <russell.ford@nyu.edu>
*                         Menglei Zhang <menglei@nyu.edu>
*/



#include <ns3/log.h>
#include "mmwave-control-messages.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("mmWaveControlMessage");

MmWaveControlMessage::MmWaveControlMessage (void)
{
  NS_LOG_INFO (this);
}

MmWaveControlMessage::~MmWaveControlMessage (void)
{
  NS_LOG_INFO (this);
}

void
MmWaveControlMessage::SetMessageType (messageType type)
{
  m_messageType = type;
}

MmWaveControlMessage::messageType
MmWaveControlMessage::GetMessageType (void) const
{
  return m_messageType;
}

MmWaveSRMessage::MmWaveSRMessage () : MmWaveControlMessage ()
{
  NS_LOG_INFO (this);
}

MmWaveSRMessage::~MmWaveSRMessage ()
{
  NS_LOG_INFO (this);
}

void
MmWaveSRMessage::SetRNTI (uint16_t rnti)
{
  m_rnti = rnti;
}

uint16_t
MmWaveSRMessage::GetRNTI () const
{
  return m_rnti;
}

MmWaveTdmaDciMessage::MmWaveTdmaDciMessage (const std::shared_ptr<DciInfoElementTdma> &dci)
  : m_dciInfoElement (dci)
{
  NS_LOG_INFO (this);
  SetMessageType (MmWaveControlMessage::DCI_TDMA);
}

MmWaveTdmaDciMessage::~MmWaveTdmaDciMessage (void)
{
  NS_LOG_INFO (this);
}

std::shared_ptr<DciInfoElementTdma>
MmWaveTdmaDciMessage::GetDciInfoElement (void)
{
  return m_dciInfoElement;
}

void
MmWaveTdmaDciMessage::SetSfnSf (SfnSf sfn)
{
  m_sfnSf = sfn;
}

SfnSf
MmWaveTdmaDciMessage::GetSfnSf (void)
{
  return m_sfnSf;
}

void
MmWaveTdmaDciMessage::SetKDelay (uint32_t delay)
{
  m_k = delay;
}

uint32_t
MmWaveTdmaDciMessage::GetKDelay (void) const
{
  return m_k;
}

MmWaveDlCqiMessage::MmWaveDlCqiMessage (void)
{
  SetMessageType (MmWaveControlMessage::DL_CQI);
  NS_LOG_INFO (this);
}
MmWaveDlCqiMessage::~MmWaveDlCqiMessage (void)
{
  NS_LOG_INFO (this);
}

void
MmWaveDlCqiMessage::SetDlCqi (DlCqiInfo cqi)
{
  m_cqi = cqi;
}

DlCqiInfo
MmWaveDlCqiMessage::GetDlCqi ()
{
  return m_cqi;
}

// ----------------------------------------------------------------------------------------------------------

MmWaveBsrMessage::MmWaveBsrMessage (void)
{
  SetMessageType (MmWaveControlMessage::BSR);
}


MmWaveBsrMessage::~MmWaveBsrMessage (void)
{

}

void
MmWaveBsrMessage::SetBsr (MacCeElement bsr)
{
  m_bsr = bsr;

}


MacCeElement
MmWaveBsrMessage::GetBsr (void)
{
  return m_bsr;
}

// ----------------------------------------------------------------------------------------------------------



MmWaveMibMessage::MmWaveMibMessage (void)
{
  SetMessageType (MmWaveControlMessage::MIB);
}


void
MmWaveMibMessage::SetMib (LteRrcSap::MasterInformationBlock  mib)
{
  m_mib = mib;
}

LteRrcSap::MasterInformationBlock
MmWaveMibMessage::GetMib () const
{
  return m_mib;
}


// ----------------------------------------------------------------------------------------------------------



MmWaveSib1Message::MmWaveSib1Message (void)
{
  SetMessageType (MmWaveControlMessage::SIB1);
}


void
MmWaveSib1Message::SetSib1 (LteRrcSap::SystemInformationBlockType1 sib1)
{
  m_sib1 = sib1;
}

void
MmWaveSib1Message::SetTddPattern(const std::vector<LteNrTddSlotType> &pattern)
{
  m_pattern = pattern;
}

LteRrcSap::SystemInformationBlockType1
MmWaveSib1Message::GetSib1 () const
{
  return m_sib1;
}

const std::vector<LteNrTddSlotType> &
MmWaveSib1Message::GetTddPattern() const
{
  return m_pattern;
}

// ----------------------------------------------------------------------------------------------------------

MmWaveRachPreambleMessage::MmWaveRachPreambleMessage (void)
{
  SetMessageType (MmWaveControlMessage::RACH_PREAMBLE);
}

void
MmWaveRachPreambleMessage::SetRapId (uint32_t rapId)
{
  m_rapId = rapId;
}

uint32_t
MmWaveRachPreambleMessage::GetRapId () const
{
  return m_rapId;
}

// ----------------------------------------------------------------------------------------------------------


MmWaveRarMessage::MmWaveRarMessage (void)
{
  SetMessageType (MmWaveControlMessage::RAR);
}


void
MmWaveRarMessage::SetRaRnti (uint16_t raRnti)
{
  m_raRnti = raRnti;
}

uint16_t
MmWaveRarMessage::GetRaRnti () const
{
  return m_raRnti;
}


void
MmWaveRarMessage::AddRar (Rar rar)
{
  m_rarList.push_back (rar);
}

std::list<MmWaveRarMessage::Rar>::const_iterator
MmWaveRarMessage::RarListBegin () const
{
  return m_rarList.begin ();
}

std::list<MmWaveRarMessage::Rar>::const_iterator
MmWaveRarMessage::RarListEnd () const
{
  return m_rarList.end ();
}

MmWaveDlHarqFeedbackMessage::MmWaveDlHarqFeedbackMessage (void)
{
  SetMessageType (MmWaveControlMessage::DL_HARQ);
}


MmWaveDlHarqFeedbackMessage::~MmWaveDlHarqFeedbackMessage (void)
{

}

void
MmWaveDlHarqFeedbackMessage::SetDlHarqFeedback (DlHarqInfo m)
{
  m_dlHarqInfo = m;
}


DlHarqInfo
MmWaveDlHarqFeedbackMessage::GetDlHarqFeedback (void)
{
  return m_dlHarqInfo;
}

std::ostream &
operator<< (std::ostream &os, const LteNrTddSlotType &item)
{
  switch (item)
    {
    case LteNrTddSlotType::DL:
      os << "DL";
      break;
    case LteNrTddSlotType::F:
      os << "F";
      break;
    case LteNrTddSlotType::S:
      os << "S";
      break;
    case LteNrTddSlotType::UL:
      os << "UL";
      break;
    }
  return os;
}

}


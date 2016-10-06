/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 NITK Surathkal
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Shravya Ks <shravya.ks0@gmail.com>
 *
 */

#include "ns3/test.h"
#include "ns3/red-queue-disc.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-queue-disc-item.h"
#include "ns3/rng-seed-manager.h"

using namespace ns3;

class EcnRedQueueDiscTestItem : public Ipv4QueueDiscItem
{
public:
  EcnRedQueueDiscTestItem (Ptr<Packet> p, const Address & addr, uint16_t protocol, const Ipv4Header & header);
  virtual ~EcnRedQueueDiscTestItem ();

private:
  EcnRedQueueDiscTestItem ();
  EcnRedQueueDiscTestItem (const EcnRedQueueDiscTestItem &);
  EcnRedQueueDiscTestItem &operator = (const EcnRedQueueDiscTestItem &);
};

EcnRedQueueDiscTestItem::EcnRedQueueDiscTestItem (Ptr<Packet> p, const Address & addr, uint16_t protocol, const Ipv4Header & header)
  : Ipv4QueueDiscItem (p, addr, protocol, header)
{
}

EcnRedQueueDiscTestItem::~EcnRedQueueDiscTestItem ()
{
}

class EcnRedQueueDiscTestCase : public TestCase
{
public:
  EcnRedQueueDiscTestCase ();
  virtual void DoRun (void);
private:
  void Enqueue (Ptr<RedQueueDisc> queue, uint32_t size, uint32_t nPkt);
  void EnqueueNonEcnCapable (Ptr<RedQueueDisc> queue, uint32_t size, uint32_t nPkt);
  void RunRedTest (StringValue mode);
};

EcnRedQueueDiscTestCase::EcnRedQueueDiscTestCase ()
  : TestCase ("Test the marking behavior for IPv4 ECN for a RED queue")
{
}

void
EcnRedQueueDiscTestCase::RunRedTest (StringValue mode)
{
  uint32_t ipPayloadSize = 1000; // bytes
  // modeSize has units of packets when 'mode' is Queue::QUEUE_MODE_PACKETS
  // modeSize has units of bytes when 'mode' is Queue::QUEUE_MODE_BYTES
  uint32_t modeSize = 1;
  double minTh = 2;
  double maxTh = 5;
  uint32_t qSize = 8;
  Ptr<RedQueueDisc> queue = CreateObject<RedQueueDisc> ();
  queue->AssignStreams (1);

  // test 1: simple enqueue/dequeue with no drops or marks
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.002)), true,
                         "Verify that we can actually set the attribute QW");

  Address dest;

  if (queue->GetMode () == Queue::QUEUE_MODE_BYTES)
    {
      ipPayloadSize = 1000;
      modeSize = ipPayloadSize + 20;
      queue->SetTh (minTh * modeSize, maxTh * modeSize);
      queue->SetQueueLimit (qSize * modeSize);
    }

  Ipv4Header hdr, tmp_hdr;
  hdr.SetEcn (Ipv4Header::ECN_ECT0);

  Ptr<Packet> p1, p2, p3, p4, p5, p6, p7, p8;
  p1 = Create<Packet> (ipPayloadSize);
  p2 = Create<Packet> (ipPayloadSize);
  p3 = Create<Packet> (ipPayloadSize);
  p4 = Create<Packet> (ipPayloadSize);
  p5 = Create<Packet> (ipPayloadSize);

  queue->Initialize ();
  Ptr<QueueDiscItem> item;
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 0 * modeSize, "There should be no packets");
  item = Create<EcnRedQueueDiscTestItem> (p1, dest, 0, hdr);
  queue->Enqueue (item);
  item = Create<EcnRedQueueDiscTestItem> (p2, dest, 0, hdr);
  queue->Enqueue (item);
  item = Create<EcnRedQueueDiscTestItem> (p3, dest, 0, hdr);
  queue->Enqueue (item);
  item = Create<EcnRedQueueDiscTestItem> (p4, dest, 0, hdr);
  queue->Enqueue (item);
  item = Create<EcnRedQueueDiscTestItem> (p5, dest, 0, hdr);
  queue->Enqueue (item);
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 5 * modeSize, "There should be five packets");

  item = DynamicCast<Ipv4QueueDiscItem> (queue->Dequeue ());
  NS_TEST_EXPECT_MSG_EQ ((item != 0), true, "Remove the first packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 4 * modeSize, "There should be four packets");
  NS_TEST_EXPECT_MSG_EQ (item->GetPacket ()->GetUid (), p1->GetUid (), "was this the first packet ?");

  NS_TEST_EXPECT_MSG_EQ ((DynamicCast<Ipv4QueueDiscItem>(item)->GetHeader()).GetEcn (), Ipv4Header::ECN_ECT0, "The packet should be marked");
   

  item = DynamicCast<Ipv4QueueDiscItem> (queue->Dequeue ());
  NS_TEST_EXPECT_MSG_EQ ((item != 0), true, "Remove the second packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 3 * modeSize, "There should be three packet");
  NS_TEST_EXPECT_MSG_EQ (item->GetPacket ()->GetUid (), p2->GetUid (), "Was this the second packet ?");

  NS_TEST_EXPECT_MSG_EQ ((DynamicCast<Ipv4QueueDiscItem>(item)->GetHeader()).GetEcn (), Ipv4Header::ECN_ECT0, "The packet should be marked");
   
  item = DynamicCast<Ipv4QueueDiscItem> (queue->Dequeue ());
  NS_TEST_EXPECT_MSG_EQ ((item != 0), true, "Remove the third packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 2 * modeSize, "There should be two packets");
  NS_TEST_EXPECT_MSG_EQ (item->GetPacket ()->GetUid (), p3->GetUid (), "Was this the third packet ?");

  DynamicCast<Ipv4QueueDiscItem> (queue->Dequeue ());
  DynamicCast<Ipv4QueueDiscItem> (queue->Dequeue ());

  item = DynamicCast<Ipv4QueueDiscItem> (queue->Dequeue ());
  NS_TEST_EXPECT_MSG_EQ ((item == 0), true, "There are no packets");

  //test2 : Test with packets which are not ECN-Capable

  queue = CreateObject<RedQueueDisc> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.002)), true,
                         "Verify that we can actually set the attribute QW");

  if (queue->GetMode () == Queue::QUEUE_MODE_BYTES)
    {
      ipPayloadSize = 1000;
      modeSize = ipPayloadSize + 20;
      queue->SetTh (minTh * modeSize, maxTh * modeSize);
      queue->SetQueueLimit (qSize * modeSize);
    }

  p1 = Create<Packet> (ipPayloadSize);
  p2 = Create<Packet> (ipPayloadSize);
  p3 = Create<Packet> (ipPayloadSize);
  p4 = Create<Packet> (ipPayloadSize);
  p5 = Create<Packet> (ipPayloadSize);

  Ipv4Header hdr1;

  queue->Initialize ();
  item = Create<EcnRedQueueDiscTestItem> (p1, dest, 0, hdr1);
  queue->Enqueue (item);
  item = Create<EcnRedQueueDiscTestItem> (p2, dest, 0, hdr1);
  queue->Enqueue (item);
  item = Create<EcnRedQueueDiscTestItem> (p3, dest, 0, hdr1);
  queue->Enqueue (item);
  item = Create<EcnRedQueueDiscTestItem> (p4, dest, 0, hdr1);
  queue->Enqueue (item);
  item = Create<EcnRedQueueDiscTestItem> (p5, dest, 0, hdr1);
  queue->Enqueue (item);
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 5 * modeSize, "There should be five packets");

  item = DynamicCast<Ipv4QueueDiscItem> (queue->Dequeue ());
  NS_TEST_EXPECT_MSG_EQ ((item != 0), true, "Remove the first packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 4 * modeSize, "There should be four packets");
  NS_TEST_EXPECT_MSG_EQ (item->GetPacket ()->GetUid (), p1->GetUid (), "was this the first packet ?");

  if (item->GetPacket ()->GetSize ())
    {
      item->GetPacket ()->PeekHeader (tmp_hdr);
      NS_TEST_EXPECT_MSG_EQ (tmp_hdr.GetEcn (), Ipv4Header::ECN_NotECT, "The packet should not be marked");
    }

  item = DynamicCast<Ipv4QueueDiscItem> (queue->Dequeue ());
  NS_TEST_EXPECT_MSG_EQ ((item != 0), true, "Remove the second packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 3 * modeSize, "There should be  three packets");
  NS_TEST_EXPECT_MSG_EQ (item->GetPacket ()->GetUid (), p2->GetUid (), "Was this the second packet ?");

  if (item->GetPacket ()->GetSize ())
    {
      item->GetPacket ()->PeekHeader (tmp_hdr);
      NS_TEST_EXPECT_MSG_EQ (tmp_hdr.GetEcn (), Ipv4Header::ECN_NotECT, "The packet should not be marked");
    }

  item = DynamicCast<Ipv4QueueDiscItem> (queue->Dequeue ());
  NS_TEST_EXPECT_MSG_EQ ((item != 0), true, "Remove the third packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 2 * modeSize, "There should be two packets");
  NS_TEST_EXPECT_MSG_EQ (item->GetPacket ()->GetUid (), p3->GetUid (), "Was this the third packet ?");

  DynamicCast<Ipv4QueueDiscItem> (queue->Dequeue ());
  DynamicCast<Ipv4QueueDiscItem> (queue->Dequeue ());
  item = DynamicCast<Ipv4QueueDiscItem> (queue->Dequeue ());
  NS_TEST_EXPECT_MSG_EQ ((item == 0), true, "There are no packets");


  // test 3: more data, but with no marks or drops
  queue = CreateObject<RedQueueDisc> ();
  minTh = 70 * modeSize;
  maxTh = 150 * modeSize;
  qSize = 300 * modeSize;
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");

  if (queue->GetMode () == Queue::QUEUE_MODE_BYTES)
    {
      NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MeanPktSize", UintegerValue (modeSize)), true,
                             "Verify that we can actually set the attribute MeanPktSize");
    }

  queue->Initialize ();
  Enqueue (queue, ipPayloadSize, 300);
  RedQueueDisc::Stats st = queue->GetStats ();
  NS_TEST_EXPECT_MSG_EQ (st.unforcedMark, 0, "There should zero marked packets due to probability mark with this seed, run number, and stream");
  NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 0, "There should zero dropped packets due to probability mark with this seed, run number, and stream");
  NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should zero dropped packets due to hard mark with this seed, run number, and stream");
  NS_TEST_EXPECT_MSG_EQ (st.qLimDrop, 0, "There should zero dropped packets due to queue full with this seed, run number, and stream");


  //test 4: more data which are ECN capable resulting in unforced marks but no unforced drops
  queue = CreateObject<RedQueueDisc> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.020)), true,
                         "Verify that we can actually set the attribute QW");

  if (queue->GetMode () == Queue::QUEUE_MODE_BYTES)
    {
      NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MeanPktSize", UintegerValue (modeSize)), true,
                             "Verify that we can actually set the attribute MeanPktSize");
    }

  queue->Initialize ();
  Enqueue (queue, ipPayloadSize, 300);
  st = queue->GetStats ();

  if (queue->GetMode () == Queue::QUEUE_MODE_PACKETS)
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 0, "There should be no unforced dropped packets with this seed, run number, and stream");
      NS_TEST_EXPECT_MSG_EQ (st.unforcedMark, 27, "There should be 27 marked packets with this seed, run number, and stream");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should be 0 forced dropped packets with this seed, run number, and stream");
    }
  else
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 0, "There should be no unforced dropped packets with this seed, run number, and stream");
      NS_TEST_EXPECT_MSG_EQ (st.unforcedMark, 26, "There should be 26 marked packets with this seed, run number, and stream");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should be 0 forced dropped packets with this seed, run number, and stream");
    }
 
  //test 5:  more data which are not ECN capable resulting in unforced drops but no unforced marks
  queue = CreateObject<RedQueueDisc> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.020)), true,
                         "Verify that we can actually set the attribute QW");

  if (queue->GetMode () == Queue::QUEUE_MODE_BYTES)
    {
      NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MeanPktSize", UintegerValue (modeSize)), true,
                             "Verify that we can actually set the attribute MeanPktSize");
    }

  queue->Initialize ();
  EnqueueNonEcnCapable (queue, ipPayloadSize, 300);
  st = queue->GetStats ();

  if (queue->GetMode () == Queue::QUEUE_MODE_PACKETS)
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedMark, 0, "There should be no unforced mark packets with this seed, run number, and stream");
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 26, "There should be 26 unforced dropped packets with this seed, run number, and stream");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should be 0 forced dropped packets with this seed, run number, and stream");
    }
  else
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedMark, 0, "There should be no unforced marked packets with this seed, run number, and stream");
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 24, "There should be 24 unforced dropped packets with this seed, run number, and stream");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should be 0 forced dropped packets with this seed, run number, and stream");
    }

  // test 6: There can be forced drops along with unforced marks but no unforced drops when packets are ECN capable
  maxTh = 100 * modeSize;
  queue = CreateObject<RedQueueDisc> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.020)), true,
                         "Verify that we can actually set the attribute QW");

  if (queue->GetMode () == Queue::QUEUE_MODE_BYTES)
    {
      NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MeanPktSize", UintegerValue (modeSize)), true,
                             "Verify that we can actually set the attribute MeanPktSize");
    }

  queue->Initialize ();
  Enqueue (queue, ipPayloadSize, 300);
  st = queue->GetStats ();

  if (queue->GetMode () == Queue::QUEUE_MODE_PACKETS)
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 0, "There should be no unforced dropped packets with this seed, run number, and stream");
      NS_TEST_EXPECT_MSG_EQ (st.unforcedMark, 35, "There should be 35 unforced marked packets  due to probability mark with this seed, run number, and stream");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 51, "There should be 51 forced drops with this seed, run number, and stream");
    }
  else
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 0, "There should be no unforced dropped packets with this seed, run number, and stream");
      NS_TEST_EXPECT_MSG_EQ (st.unforcedMark, 36, "There should be 36 unforced marked packets  due to probability mark with this seed, run   number, and stream");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 51, "There should be 51 forced drops due to queue limit with this seed, run number, and stream");
    }
}

void
EcnRedQueueDiscTestCase::Enqueue (Ptr<RedQueueDisc> queue, uint32_t size, uint32_t nPkt)
{
  Address dest;
  Ipv4Header hdr;
  hdr.SetEcn (Ipv4Header::ECN_ECT0);
  Ptr<QueueDiscItem> item;

  for (uint32_t i = 0; i < nPkt; i++)
    {
      Ptr<Packet> p = Create<Packet> (size);
      item = Create<EcnRedQueueDiscTestItem> (p, dest, 0, hdr);
     
      queue->Enqueue (item);
    }
}

void
EcnRedQueueDiscTestCase::EnqueueNonEcnCapable (Ptr<RedQueueDisc> queue, uint32_t size, uint32_t nPkt)
{
  Address dest;
  Ipv4Header hdr;
  Ptr<QueueDiscItem> item;

  for (uint32_t i = 0; i < nPkt; i++)
    {
      Ptr<Packet> p = Create<Packet> (size);
      item = Create<EcnRedQueueDiscTestItem> (p, dest, 0, hdr);
      queue->Enqueue (item);

    }
}

void
EcnRedQueueDiscTestCase::DoRun (void)
{
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (17);
  RunRedTest (StringValue ("QUEUE_MODE_PACKETS"));
  RunRedTest (StringValue ("QUEUE_MODE_BYTES"));
  Simulator::Destroy ();

}

static class EcnRedQueueDiscTestSuite : public TestSuite
{
public:
  EcnRedQueueDiscTestSuite ()
    : TestSuite ("ecn-red-queue-disc", UNIT)
  {
    AddTestCase (new EcnRedQueueDiscTestCase (), TestCase::QUICK);
  }
} g_redQueueTestSuite;

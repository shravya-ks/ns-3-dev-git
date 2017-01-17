#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/config-store-module.h"

using namespace ns3;


/*
         DUMBELL TOPOLOGY
      N1            N4
       \            /
        \          /
  N2-----R1-------R2-----N5
        /          \
       /            \
      N3            N6
*/

NS_LOG_COMPONENT_DEFINE ("ecn-tcp-test");


NodeContainer N1R1;
NodeContainer R1N2;

Ipv4InterfaceContainer In1r1;
Ipv4InterfaceContainer Ir1n2;

class EcnTcpTestCase : public TestCase
{
public:
  EcnTcpTestCase ();
  TestVectors<uint8_t> m_responses;
  virtual ~ EcnTcpTestCase ();
  virtual void DoRun (void);
private:
  uint32_t numSenderRecv;
  uint32_t numReceiverRecv;
/*static void SinkTx ( Ptr<const Packet> p,const TcpHeader& tcp, Ptr<const TcpSocketBase> tsb)
{
  NS_LOG_INFO (tcp);
}

static void SenderTx ( Ptr<const Packet> p,const TcpHeader& tcp, Ptr<const TcpSocketBase> tsb)
{
  NS_LOG_INFO (tcp);
}*/

void SinkRx ( Ptr<const Packet> p,const TcpHeader& tcp, Ptr<const TcpSocketBase> tsb)
{
  if (numReceiverRecv == 0)
    {
      //NS_TEST_EXPECT_MSG_EQ (tcp.GetFlags () & TcpHeader::SYN, true, "SYN should be enabled in first packet recceived");
    }
  else if (numReceiverRecv == 1)
    {
      //NS_TEST_EXPECT_MSG_EQ (tcp.GetFlags () & TcpHeader::ACK, true, "ACK should be enabled in second packet recceived");
    }
  NS_LOG_INFO (tcp);
  numReceiverRecv++;
}

void SenderRx ( Ptr<const Packet> p,const TcpHeader& tcp, Ptr<const TcpSocketBase> tsb)
{
  if (numSenderRecv == 0)
    {
      //NS_TEST_EXPECT_MSG_EQ ((tcp.GetFlags () & TcpHeader::SYN) && (tcp.GetFlags () & TcpHeader::ACK), true, "SYN should be enabled in first packet recceived");
    }
  NS_LOG_INFO (tcp);
  numSenderRecv++;
}

/*static void TraceTx ()
{
  Config::ConnectWithoutContext ("/NodeList/2/$ns3::TcpL4Protocol/SocketList/0/Tx", MakeCallback (&EcnTcpTestCase::SinkTx));
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/Tx", MakeCallback (&EcnTcpTestCase::SenderTx));
}*/

void TraceRx ()
{
  Config::ConnectWithoutContext ("/NodeList/2/$ns3::TcpL4Protocol/SocketList/0/Rx", MakeCallback (&EcnTcpTestCase::SinkRx));
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/Rx", MakeCallback (&EcnTcpTestCase::SenderRx));
}
  void RunEcnTest ();
};


EcnTcpTestCase::EcnTcpTestCase ()
  : TestCase ("Sanity check on the ECN implementation for TCP")
{
//numSenderSent = 0;
numSenderRecv = 0;
//numReceiverSent = 0;
numReceiverRecv = 0;
}

EcnTcpTestCase:: ~EcnTcpTestCase ()
{
}

void
EcnTcpTestCase::RunEcnTest ()
{
  Time::SetResolution (Time::NS);
  LogComponentEnable("ecn-tcp-test", LOG_LEVEL_INFO);
  // Main Container of all nodes in the topology
  NS_LOG_INFO ("Create nodes");
  NodeContainer nodes;


  nodes.Create (3);
  Names::Add ( "N1", nodes.Get (0));
  Names::Add ( "R1", nodes.Get (1));
  Names::Add ( "N2", nodes.Get (2));
  
  N1R1 = NodeContainer (nodes.Get (0), nodes.Get (1));
  R1N2 = NodeContainer (nodes.Get (1), nodes.Get (2));
 
  NS_LOG_INFO ("Install internet stack on all nodes.");
  InternetStackHelper stack;
  stack.Install (nodes);

  // Setting the channel properties

  NS_LOG_INFO ("Create channels");
  PointToPointHelper p2p;

  TrafficControlHelper tchBottleneck;
  tchBottleneck.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "Limit", UintegerValue (850));

  p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("0ms"));
  NetDeviceContainer devN1R1 = p2p.Install (N1R1);


  p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("12ms"));
  NetDeviceContainer devR1N2 = p2p.Install (R1N2);

  NS_LOG_INFO ("Assign IP Addresses");
  Ipv4AddressHelper ipv4;

  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  In1r1 = ipv4.Assign (devN1R1);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  Ir1n2 = ipv4.Assign (devR1N2);

  // Set up the routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  OnOffHelper clientHelper1 ("ns3::TcpSocketFactory", Address ());
  clientHelper1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  clientHelper1.SetAttribute ("DataRate", DataRateValue (DataRate ("10Mb/s")));
  clientHelper1.SetAttribute ("PacketSize", UintegerValue (1000));

  uint16_t port = 50000;
  ApplicationContainer clientApps1;
  AddressValue remoteAddress
  (InetSocketAddress (Ir1n2.GetAddress (1), port));
  clientHelper1.SetAttribute ("Remote", remoteAddress);
  clientApps1.Add (clientHelper1.Install (N1R1.Get (0)));
  clientApps1.Start (Seconds (0.));
  clientApps1.Stop (Seconds (18.0));

  Address sinkAddress (InetSocketAddress (Ir1n2.GetAddress (1), port));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (2));
  sinkApps.Start (Seconds (0.));
  sinkApps.Stop (Seconds (20.)); 

  //Simulator::Schedule (Seconds (0.0000001), &EcnTcpTestCase::TraceTx);
  Simulator::Schedule (Seconds (0.0000001), &EcnTcpTestCase::TraceRx);

  Simulator::Run ();
  Simulator::Destroy ();
}

void EcnTcpTestCase::DoRun ()
{
}





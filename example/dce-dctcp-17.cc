/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 NITK Surathkal
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
 * Authors: Apoorva Bhargava <apoorvabhargava13@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

// The network topology used in this example is based on the Fig. 17 described in
// Mohammad Alizadeh, Albert Greenberg, David A. Maltz, Jitendra Padhye,
// Parveen Patel, Balaji Prabhakar, Sudipta Sengupta, and Murari Sridharan.
// "Data Center TCP (DCTCP)." In ACM SIGCOMM Computer Communication Review,
// Vol. 40, No. 4, pp. 63-74. ACM, 2010.

#include <iostream>
#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/applications-module.h"
#include "ns3/packet-sink.h"
#include "ns3/traffic-control-module.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/callback.h"
#include "ns3/dce-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/config-store-module.h"

using namespace ns3;

std::vector<std::stringstream> filePlotQueue;
Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
std::string dir = "results/gfc-dctcp/";
double stopTime = 20;
NodeContainer senderNodes, receiverNodes;

// Checks the queue size for router T1 after every 0.001 seconds
void
CheckQueueSize (Ptr<QueueDisc> queue)
{
  uint32_t qSize = queue->GetCurrentSize ().GetValue ();

  // check queue size every 1/100 of a second
  Simulator::Schedule (Seconds (0.001), &CheckQueueSize, queue);

  std::ofstream fPlotQueue (std::stringstream (dir + "queue-0.plotme").str ().c_str (), std::ios::out | std::ios::app);
  fPlotQueue << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;
  fPlotQueue.close ();
}

// Checks queue size for router Scorpion after every 0.001 seconds
void
CheckQueueSize1 (Ptr<QueueDisc> queue)
{
  uint32_t qSize = queue->GetCurrentSize ().GetValue ();

  // check queue size every 1/100 of a second
  Simulator::Schedule (Seconds (0.001), &CheckQueueSize1, queue);

  std::ofstream fPlotQueue (std::stringstream (dir + "queue-1.plotme").str ().c_str (), std::ios::out | std::ios::app);
  fPlotQueue << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;
  fPlotQueue.close ();
}

// Checks queue size for router T2 after every 0.001 seconds
void
CheckQueueSize2 (Ptr<QueueDisc> queue)
{
  uint32_t qSize = queue->GetCurrentSize ().GetValue ();

  // check queue size every 1/100 of a second
  Simulator::Schedule (Seconds (0.001), &CheckQueueSize2, queue);

  std::ofstream fPlotQueue (std::stringstream (dir + "queue-2.plotme").str ().c_str (), std::ios::out | std::ios::app);
  fPlotQueue << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;
  fPlotQueue.close ();
}

static void
DropAtQueue (Ptr<OutputStreamWrapper> stream, Ptr<const QueueDiscItem> item)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " 1" << std::endl;
}

static void
MarkAtQueue (Ptr<OutputStreamWrapper> stream, Ptr<const QueueDiscItem> item, const char* reason)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " 1" << std::endl;
}

// Install PacketSinkHelper on a particular node
void InstallPacketSink (Ptr<Node> node, uint16_t port, std::string sock_factory)
{
  PacketSinkHelper sink (sock_factory,
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (node);
  sinkApps.Start (Seconds (10.0));
  sinkApps.Stop (Seconds (stopTime));
}

// Install Bulk Send Application on the particular node and starts the application randomly between 10 to 11 seconds
void InstallBulkSend (Ptr<Node> node, Ipv4Address address, uint16_t port, std::string sock_factory)
{
  BulkSendHelper source (sock_factory,
                         InetSocketAddress (address, port));

  source.SetAttribute ("MaxBytes", UintegerValue (0));
  ApplicationContainer sourceApps = source.Install (node);
  Time timeToStart = Seconds (uv->GetValue (10, 11));
  sourceApps.Start (timeToStart);
  sourceApps.Stop (Seconds (stopTime));
}

/*
Runs ss command on a particular node with linux stack to
get socket stats of that node, output of this command is stored
in files folder of that corresponding node
*/
static void GetSSStats (Ptr<Node> node, Time at)
{
  DceApplicationHelper process;
  ApplicationContainer apps;
  process.SetBinary ("ss");
  process.SetStackSize (1 << 20);
  process.AddArgument ("-a");
  process.AddArgument ("-e");
  process.AddArgument ("-i");
  apps.Add (process.Install (node));
  apps.Start (at);
}

int main (int argc, char *argv[])
{
  uint32_t stream = 1;
  std::string sock_factory = "ns3::LinuxTcpSocketFactory";
  std::string transport_prot = "dctcp";
  std::string queue_disc_type = "RedQueueDisc";
  bool useEcn = true;

  //Enable checksum for comunication between node with linux stack and ns3 stack
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  time_t rawtime;
  struct tm * timeinfo;
  char buffer[80];

  time (&rawtime);
  timeinfo = localtime (&rawtime);

  strftime (buffer,sizeof(buffer),"%d-%m-%Y-%I-%M-%S",timeinfo);
  std::string currentTime (buffer);

  // Enable this program to read and parse command line arguments
  CommandLine cmd;
  cmd.AddValue ("stream", "Seed value for random variable", stream);
  cmd.AddValue ("transport_prot", "Linux network protocol to use: reno, "
                "hybla, highspeed, htcp, vegas, scalable, veno, "
                "bic, yeah, illinois, westwood, lp", transport_prot);
  cmd.AddValue ("queue_disc_type", "Queue disc type for gateway (e.g. ns3::CoDelQueueDisc)", queue_disc_type);
  cmd.AddValue ("useEcn", "Use ECN", useEcn);
  cmd.AddValue ("stopTime", "Stop time for applications / simulation time will be stopTime", stopTime);
  cmd.Parse (argc,argv);

  uv->SetStream (stream);
  queue_disc_type = std::string ("ns3::") + queue_disc_type;

  TypeId qdTid;
  NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (queue_disc_type, &qdTid), "TypeId " << queue_disc_type << " not found");

  NodeContainer S1, S2, S3, R1, R2, T, Scorp;
  // Create router nodes
  T.Create (2);
  Scorp.Create (1);

  // Create sender nodes
  S1.Create (10);
  S2.Create (20);
  S3.Create (10);

  // Create receiver nodes
  R2.Create (20);
  R1.Create (1);

  senderNodes.Add (S1);
  senderNodes.Add (S2);
  senderNodes.Add (S3);
  receiverNodes.Add (R1);
  receiverNodes.Add (R2);

  // Create a point-to-point channel for senders and receiver to router and configure its attributes
  PointToPointHelper pointToPointSR;
  pointToPointSR.SetDeviceAttribute ("DataRate", StringValue ("1000Mbps"));
  pointToPointSR.SetChannelAttribute ("Delay", StringValue ("0.05ms"));

  // Create a point-to-point channel for routers and configure its attributes
  PointToPointHelper pointToPointT;
  pointToPointT.SetDeviceAttribute ("DataRate", StringValue ("10000Mbps"));
  pointToPointT.SetChannelAttribute ("Delay", StringValue ("0.05ms"));

  PointToPointHelper pointToPointT1;
  pointToPointT1.SetDeviceAttribute ("DataRate", StringValue ("10000Mbps"));
  pointToPointT1.SetChannelAttribute ("Delay", StringValue ("0.05ms"));

  // Installing point-to-point channel between router T1 and Scorp and router T2 and Scorp
  NetDeviceContainer T1ScorpDev, T2ScorpDev, S1T1Dev, S2T1Dev, S3T2Dev, R1T2Dev, R2T2Dev;
  T1ScorpDev = pointToPointT.Install (T.Get (0), Scorp.Get (0));
  T2ScorpDev = pointToPointT1.Install (Scorp.Get (0), T.Get (1));

  //Installing point-to-point channel between receiver R1 and router T2
  R1T2Dev = pointToPointSR.Install (R1.Get (0), T.Get (1));

  // Installing point-to-point channel between S1 senders and T1 and S3 senders and T2
  for (uint32_t i = 0; i < S1.GetN (); i++)
    {
      S1T1Dev.Add (pointToPointSR.Install (S1.Get (i), T.Get (0)));
      S3T2Dev.Add (pointToPointSR.Install (S3.Get (i), T.Get (1)));
    }

  // Installing point-to-point channel between S2 sender and router T1
  for (uint32_t i = 0; i < S2.GetN (); i++)
    {
      S2T1Dev.Add (pointToPointSR.Install (S2.Get (i), T.Get (0)));
    }

  // Installing point-to-point channel between R2 receiver and router T2
  for (uint32_t i = 0; i < R2.GetN (); i++)
    {
      R2T2Dev.Add (pointToPointSR.Install (R2.Get (i), T.Get (1)));
    }

  // Create object of DceManagerHelper
  DceManagerHelper dceManager;

  // Set network stack and task manager for DCE Manager
  dceManager.SetTaskManagerAttribute ("FiberManagerType",
                                      StringValue ("UcontextFiberManager"));
  dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory",
                              "Library", StringValue ("liblinux.so"));

  // Install linux network stack on all sender and receiver nodes
  LinuxStackHelper linuxStack;
  linuxStack.Install (S1);
  linuxStack.Install (S2);
  linuxStack.Install (S3);
  linuxStack.Install (R1);
  linuxStack.Install (R2);

  // Install ns-3 network stack on all routers
  InternetStackHelper internetStack;
  internetStack.Install (T);
  internetStack.Install (Scorp);

  //Assign IP address to net devices of all the links and each link is kept in a different network
  Ipv4AddressHelper address;
  Ipv4InterfaceContainer S1Int, S2Int, S3Int, R1Int, R2Int, T1Int,T2Int;

  address.SetBase ("10.1.0.0", "255.255.255.0");

  for (uint32_t i = 0; i < S1T1Dev.GetN (); i = i + 2)
    {
      address.NewNetwork ();
      NetDeviceContainer S1iT1i;
      S1iT1i.Add (S1T1Dev.Get (i));
      S1iT1i.Add (S1T1Dev.Get (i + 1));
      S1Int.Add (address.Assign (S1iT1i));
    }
  address.SetBase ("10.2.0.0", "255.255.255.0");


  for (uint32_t i = 0; i < S2T1Dev.GetN (); i = i + 2)
    {
      address.NewNetwork ();
      NetDeviceContainer S2iT1i;
      S2iT1i.Add (S2T1Dev.Get (i));
      S2iT1i.Add (S2T1Dev.Get (i + 1));
      S2Int.Add (address.Assign (S2iT1i));
    }

  address.SetBase ("10.3.0.0", "255.255.255.0");

  for (uint32_t i = 0; i < S3T2Dev.GetN (); i = i + 2)
    {
      address.NewNetwork ();
      NetDeviceContainer S3iT2i;
      S3iT2i.Add (S3T2Dev.Get (i));
      S3iT2i.Add (S3T2Dev.Get (i + 1));
      S3Int.Add (address.Assign (S3iT2i));
    }

  address.SetBase ("10.4.0.0", "255.255.255.0");
  R1Int = address.Assign (R1T2Dev);
  address.SetBase ("10.5.0.0", "255.255.255.0");


  for (uint32_t i = 0; i < R2T2Dev.GetN (); i = i + 2)
    {
      address.NewNetwork ();
      NetDeviceContainer R2iT2i;
      R2iT2i.Add (R2T2Dev.Get (i));
      R2iT2i.Add (R2T2Dev.Get (i + 1));
      R2Int.Add (address.Assign (R2iT2i));
    }

  address.SetBase ("10.6.0.0", "255.255.255.0");
  T1Int = address.Assign (T1ScorpDev);

  address.SetBase ("10.7.0.0", "255.255.255.0");
  T2Int = address.Assign (T2ScorpDev);

  // Install DCE Manager on all the nodes
  dceManager.Install (S1);
  dceManager.Install (S2);
  dceManager.Install (S3);
  dceManager.Install (R1);
  dceManager.Install (R2);
  dceManager.Install (T);
  dceManager.Install (Scorp);
  // Activate IP forwarding in linux stack of all senders and receivers
  linuxStack.SysctlSet (S1, ".net.ipv4.conf.default.forwarding", "1");
  linuxStack.SysctlSet (S2, ".net.ipv4.conf.default.forwarding", "1");
  linuxStack.SysctlSet (S3, ".net.ipv4.conf.default.forwarding", "1");
  linuxStack.SysctlSet (R1, ".net.ipv4.conf.default.forwarding", "1");
  linuxStack.SysctlSet (R2, ".net.ipv4.conf.default.forwarding", "1");
  // Set TCP congestion control algorithm in linux stack of all senders and receivers
  linuxStack.SysctlSet (S1, ".net.ipv4.tcp_congestion_control", transport_prot);
  linuxStack.SysctlSet (S2, ".net.ipv4.tcp_congestion_control", transport_prot);
  linuxStack.SysctlSet (S3, ".net.ipv4.tcp_congestion_control", transport_prot);
  linuxStack.SysctlSet (R1, ".net.ipv4.tcp_congestion_control", transport_prot);
  linuxStack.SysctlSet (R2, ".net.ipv4.tcp_congestion_control", transport_prot);
  // Eanble ECN in linux stack of all senders and receivers
  linuxStack.SysctlSet (S1, ".net.ipv4.tcp_ecn", "1");
  linuxStack.SysctlSet (S2, ".net.ipv4.tcp_ecn", "1");
  linuxStack.SysctlSet (S3, ".net.ipv4.tcp_ecn", "1");
  linuxStack.SysctlSet (R1, ".net.ipv4.tcp_ecn", "1");
  linuxStack.SysctlSet (R2, ".net.ipv4.tcp_ecn", "1");

  Ptr<Ipv4> ipv4T1 = T.Get (0)->GetObject<Ipv4> ();
  Ptr<Ipv4> ipv4T2 = T.Get (1)->GetObject<Ipv4> ();
  Ptr<Ipv4> ipv4Scorp = Scorp.Get (0)->GetObject<Ipv4> ();
  Ipv4StaticRoutingHelper routingHelper;
  Ptr<Ipv4StaticRouting> staticRoutingT1 = routingHelper.GetStaticRouting (ipv4T1);
  Ptr<Ipv4StaticRouting> staticRoutingT2 = routingHelper.GetStaticRouting (ipv4T2);
  Ptr<Ipv4StaticRouting> staticRoutingScorp = routingHelper.GetStaticRouting (ipv4Scorp);

  // Static routing table for router T1
  staticRoutingT1->AddNetworkRouteTo (Ipv4Address ("10.4.0.0"), Ipv4Mask ("255.255.255.0"), Ipv4Address ("10.6.0.2"), 31);
  address.SetBase ("10.5.0.0", "255.255.255.0");
  for (uint32_t i = 0; i < R2T2Dev.GetN (); i = i + 2)
    {
      Ipv4Address networkAddress = address.NewNetwork ();
      staticRoutingT1->AddNetworkRouteTo (Ipv4Address (networkAddress), Ipv4Mask ("255.255.255.0"), Ipv4Address ("10.6.0.2"), 31);
    }

  // Static routing table for router Scorp
  staticRoutingScorp->AddNetworkRouteTo (Ipv4Address ("10.4.0.0"), Ipv4Mask ("255.255.255.0"), Ipv4Address ("10.7.0.2"), 2);
  address.SetBase ("10.5.0.0", "255.255.255.0");
  for (uint32_t i = 0; i < R2T2Dev.GetN (); i = i + 2)
    {
      Ipv4Address networkAddress = address.NewNetwork ();
      staticRoutingScorp->AddNetworkRouteTo (Ipv4Address (networkAddress), Ipv4Mask ("255.255.255.0"), Ipv4Address ("10.7.0.2"), 2);
    }
  address.SetBase ("10.1.0.0", "255.255.255.0");
  for (uint32_t i = 0; i < S1T1Dev.GetN (); i = i + 2)
    {
      Ipv4Address networkAddress = address.NewNetwork ();
      staticRoutingScorp->AddNetworkRouteTo (Ipv4Address (networkAddress), Ipv4Mask ("255.255.255.0"), Ipv4Address ("10.6.0.1"), 1);
    }
  address.SetBase ("10.2.0.0", "255.255.255.0");
  for (uint32_t i = 0; i < S2T1Dev.GetN (); i = i + 2)
    {
      Ipv4Address networkAddress = address.NewNetwork ();
      staticRoutingScorp->AddNetworkRouteTo (Ipv4Address (networkAddress), Ipv4Mask ("255.255.255.0"), Ipv4Address ("10.6.0.1"), 1);
    }

  // Static routing table for T2
  address.SetBase ("10.1.0.0", "255.255.255.0");
  for (uint32_t i = 0; i < S1T1Dev.GetN (); i = i + 2)
    {
      Ipv4Address networkAddress = address.NewNetwork ();
      staticRoutingT2->AddNetworkRouteTo (Ipv4Address (networkAddress), Ipv4Mask ("255.255.255.0"), Ipv4Address ("10.7.0.1"), 32);
    }
  address.SetBase ("10.2.0.0", "255.255.255.0");
  for (uint32_t i = 0; i < S2T1Dev.GetN (); i = i + 2)
    {
      Ipv4Address networkAddress = address.NewNetwork ();
      staticRoutingT2->AddNetworkRouteTo (Ipv4Address (networkAddress), Ipv4Mask ("255.255.255.0"), Ipv4Address ("10.7.0.1"), 32);
    }

  std::ostringstream cmd_oss;

  //Set default route for S1 senders
  for (uint32_t i = 0; i < 10; i = i + 1)
    {
      cmd_oss.str ("");
      cmd_oss << "route add default via " << (T.Get (0)->GetObject<Ipv4> ())->GetAddress (i + 1, 0).GetLocal () << " dev sim0";
      LinuxStackHelper::RunIp (S1.Get (i), Seconds (0.00001), cmd_oss.str ());
      LinuxStackHelper::RunIp (S1.Get (i), Seconds (0.00001), "link set sim0 up");
    }

  //Set default route for S2 senders
  for (uint32_t i = 0; i < 20; i = i + 1)
    {
      cmd_oss.str ("");
      cmd_oss << "route add default via " << (T.Get (0)->GetObject<Ipv4> ())->GetAddress (i + 11, 0).GetLocal () << " dev sim0";
      LinuxStackHelper::RunIp (S2.Get (i), Seconds (0.00001), cmd_oss.str ());
      LinuxStackHelper::RunIp (S2.Get (i), Seconds (0.00001), "link set sim0 up");
    }

  //Set default route for S3 senders
  for (uint32_t i = 0; i < 10; i = i + 1)
    {
      cmd_oss.str ("");
      cmd_oss << "route add default via " << (T.Get (1)->GetObject<Ipv4> ())->GetAddress (i + 1, 0).GetLocal () << " dev sim0";
      LinuxStackHelper::RunIp (S3.Get (i), Seconds (0.00001), cmd_oss.str ());
      LinuxStackHelper::RunIp (S3.Get (i), Seconds (0.00001), "link set sim0 up");
    }

  //Set default route for R1 receiver
  LinuxStackHelper::RunIp (R1.Get (0), Seconds (0.00001), "route add default via 10.4.0.2 dev sim0");
  LinuxStackHelper::RunIp (R1.Get (0), Seconds (0.00001), "link set sim0 up");

  //Set default route for R2 receivers
  for (uint32_t i = 0; i < 20; i = i + 1)
    {
      cmd_oss.str ("");
      cmd_oss << "route add default via " << (T.Get (1)->GetObject<Ipv4> ())->GetAddress (i + 12, 0).GetLocal () << " dev sim0";
      LinuxStackHelper::RunIp (R2.Get (i), Seconds (0.00001), cmd_oss.str ());
      LinuxStackHelper::RunIp (R2.Get (i), Seconds (0.00001), "link set sim0 up");
    }

  dir += (currentTime + "/");
  std::string dirToSave = "mkdir -p " + dir;
  system (dirToSave.c_str ());
  system ((dirToSave + "/pcap/").c_str ());
  system ((dirToSave + "/markTraces/").c_str ());
  system ((dirToSave + "/queueTraces/").c_str ());
  system (("cp -R PlotScripts-gfc-dctcp/* " + dir + "/pcap/").c_str ());

  // Set default parameters RED queue disc
  Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (useEcn));
  Config::SetDefault ("ns3::RedQueueDisc::ARED", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::UseHardDrop", BooleanValue (false));
  Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (1500));
  Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (65));
  Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (65));
  Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (1));
  Config::SetDefault (queue_disc_type + "::MaxSize", QueueSizeValue (QueueSize ("666p")));

  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> streamWrapper;

  TrafficControlHelper tch;
  tch.SetRootQueueDisc (queue_disc_type);

  QueueDiscContainer qd, qd1, qd2;

  // Install RED queue disc on outgoing link of T1 and get drop and mark packets stats
  tch.Uninstall (T1ScorpDev);
  qd = tch.Install (T1ScorpDev);
  Simulator::ScheduleNow (&CheckQueueSize, qd.Get (0));
  streamWrapper = asciiTraceHelper.CreateFileStream (dir + "/queueTraces/drop-0.plotme");
  qd.Get (0)->TraceConnectWithoutContext ("Drop", MakeBoundCallback (&DropAtQueue, streamWrapper));
  streamWrapper = asciiTraceHelper.CreateFileStream (dir + "/queueTraces/mark-0.plotme");
  qd.Get (0)->TraceConnectWithoutContext ("Mark", MakeBoundCallback (&MarkAtQueue, streamWrapper));

  // Install RED queue disc on outgoing link of Scorp and get drop and mark packets stats
  tch.Uninstall (T2ScorpDev);
  qd1 = tch.Install (T2ScorpDev);
  Simulator::ScheduleNow (&CheckQueueSize1, qd1.Get (0));
  streamWrapper = asciiTraceHelper.CreateFileStream (dir + "/queueTraces/drop-1.plotme");
  qd1.Get (0)->TraceConnectWithoutContext ("Drop", MakeBoundCallback (&DropAtQueue, streamWrapper));
  streamWrapper = asciiTraceHelper.CreateFileStream (dir + "/queueTraces/mark-1.plotme");
  qd1.Get (0)->TraceConnectWithoutContext ("Mark", MakeBoundCallback (&MarkAtQueue, streamWrapper));

  // Install RED queue disc on outgoing link of T2 and get drop and mark packets stats
  tch.Uninstall (R1T2Dev.Get (1));
  qd2 = tch.Install (R1T2Dev.Get (1));
  Simulator::ScheduleNow (&CheckQueueSize2, qd2.Get (0));
  streamWrapper = asciiTraceHelper.CreateFileStream (dir + "/queueTraces/drop-2.plotme");
  qd2.Get (0)->TraceConnectWithoutContext ("Drop", MakeBoundCallback (&DropAtQueue, streamWrapper));
  streamWrapper = asciiTraceHelper.CreateFileStream (dir + "/queueTraces/mark-2.plotme");
  qd2.Get (0)->TraceConnectWithoutContext ("Mark", MakeBoundCallback (&MarkAtQueue, streamWrapper));

  // Set queue size for outgoing link of router T1
  Config::Set ("/$ns3::NodeListPriv/NodeList/0/$ns3::Node/$ns3::TrafficControlLayer/RootQueueDiscList/0/$" + queue_disc_type + "/MaxSize", QueueSizeValue (QueueSize ("666p")));
  // Set queue size for outgoing link of router T2
  Config::Set ("/$ns3::NodeListPriv/NodeList/1/$ns3::Node/$ns3::TrafficControlLayer/RootQueueDiscList/1/$" + queue_disc_type + "/MaxSize", QueueSizeValue (QueueSize ("666p")));
  // Set queue size for outgoing link of router Scorp
  Config::Set ("/$ns3::NodeListPriv/NodeList/2/$ns3::Node/$ns3::TrafficControlLayer/RootQueueDiscList/1/$" + queue_disc_type + "/MaxSize", QueueSizeValue (QueueSize ("666p")));

  uint16_t port = 50000;
  //Install sink applications on R1
  for (int i = 0; i < S1.GetN () + S3.GetN (); i++)
    {
      InstallPacketSink (R1.Get (0), port + i, sock_factory);
    }

  //Install BulkSend application on S1 and S3
  for (int i = 0; i < S1.GetN (); i++)
    {
      InstallBulkSend (S1.Get (i), R1Int.GetAddress (0), port + i, sock_factory);
    }

  //Install BulkSend application on S3
  for (int i = 0; i < S3.GetN (); i++)
    {
      InstallBulkSend (S3.Get (i), R1Int.GetAddress (0), port + i + 10, sock_factory);
    }

  //Install Sink applications on R2
  for (uint32_t i = 0; i < R2.GetN (); i++)
    {
      InstallPacketSink (R2.Get (i), port, sock_factory);
    }

  //Install BulkSend application on S2
  for (int i = 0; i < S2.GetN (); i++)
    {
      InstallBulkSend (S2.Get (i), R2Int.GetAddress (i * 2), port, sock_factory);
    }

  pointToPointSR.EnablePcapAll (dir + "pcap/N", true);

  // Calls GetSSStats function on sender nodes after every 0.1 seconds
  for (int j = 0; j < 40; j++)
    {
      for (float i = 10.0; i < stopTime; i = i + 0.1)
        {
          GetSSStats (senderNodes.Get (j), Seconds (i));
        }
    }

  Simulator::Stop (Seconds (stopTime));
  Simulator::Run ();

  // Store queue statistics in queueStats.txt file
  std::ofstream myfile;
  myfile.open (dir + "queueStats.txt", std::fstream::in | std::fstream::out | std::fstream::app);
  myfile << std::endl;
  myfile << "Stat for Queue 0";
  myfile << qd.Get (0)->GetStats ();
  myfile << "Stat for Queue 1";
  myfile << qd1.Get (0)->GetStats ();
  myfile << "Stat for Queue 2";
  myfile << qd2.Get (0)->GetStats ();
  myfile.close ();

  // Store configuration of the simulator in config.txt file
  myfile.open (dir + "config.txt", std::fstream::in | std::fstream::out | std::fstream::app);
  myfile << "useEcn " << useEcn << "\n";
  myfile << "queue_disc_type " << queue_disc_type << "\n";
  myfile << "stream  " << stream << "\n";
  myfile << "transport_prot " << transport_prot << "\n";
  myfile << "stopTime " << stopTime << "\n";
  myfile.close ();

  Simulator::Destroy ();
  return 0;

}

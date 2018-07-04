#include <iostream>
#include <fstream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/dce-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("DceTraceFullDumbbell");

NodeContainer linuxNodes;
std::string dir = "Plots/";

void
qlen (Ptr<QueueDisc> queue)
{
  uint32_t qSize = queue->GetCurrentSize ().GetValue ();
  Simulator::Schedule (Seconds (0.1), &qlen, queue);
  std::ofstream fPlotQueue (std::stringstream (dir +"/queueTraces/A.plotme").str ().c_str (), std::ios::out | std::ios::app);
  fPlotQueue << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;
  fPlotQueue.close ();
}

static void GetSSStats (Ptr<Node> node, Time at, std::string stack)
{
  if(stack == "linux")
  {
    DceApplicationHelper process;
    ApplicationContainer apps;
    process.SetBinary ("ss");
    process.SetStackSize (1 << 20);
    process.AddArgument ("-a");
    process.AddArgument ("-e");
    process.AddArgument ("-i");
    apps.Add(process.Install (node));
    apps.Start(at);
  }
}

int main (int argc, char *argv[])
{
  char buffer[80];
  time_t rawtime;
  struct tm * timeinfo;
  time (&rawtime);
  timeinfo = localtime(&rawtime);

  strftime(buffer,sizeof(buffer),"%d-%m-%Y-%I-%M-%S",timeinfo);
  std::string currentTime (buffer);

  bool pcap = true;
  std::string stack = "linux";
  std::string sock_factory = "ns3::TcpSocketFactory";
  float duration = 10;
  std::string transport_prot="dctcp";
  std::string queue_disc_type = "ns3::RedQueueDisc";
  float start_time = 10.1;

  //Enable checksum if linux and ns3 node communicate
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));


  CommandLine cmd;
  cmd.AddValue ("pcap", "Enable PCAP", pcap);
  cmd.AddValue ("stack", "Network stack: either ns3 or Linux", stack);
  cmd.AddValue ("start_time", "Time to start the flows", start_time);
  cmd.AddValue ("duration", "Time to allow flows to run in seconds", duration);
  cmd.Parse (argc, argv);

  float stop_time = start_time + duration;

  if (stack != "ns3" && stack != "linux")
    {
      std::cout << "ERROR: " <<  stack << " is not available" << std::endl; 
    }
  NodeContainer nodes, routerNodes, linuxNodes;
  nodes.Create (4);
  linuxNodes.Add (nodes.Get (0));
  linuxNodes.Add (nodes.Get (3));
  routerNodes.Add (nodes.Get (1));
  routerNodes.Add (nodes.Get (2));
 

  PointToPointHelper link;
  link.SetDeviceAttribute ("DataRate", StringValue ("150Mbps"));
  link.SetChannelAttribute ("Delay", StringValue ("5ms"));

  PointToPointHelper bottleNeckLink;
  bottleNeckLink.SetDeviceAttribute ("DataRate", StringValue ("50Mbps"));
  bottleNeckLink.SetChannelAttribute ("Delay", StringValue ("1ms"));

  NetDeviceContainer leftToRouterDevices, routerToRightDevices, bottleNeckDevices;
  leftToRouterDevices = link.Install (linuxNodes.Get (0), routerNodes.Get (0));
  bottleNeckDevices = bottleNeckLink.Install (routerNodes.Get (0), routerNodes.Get (1));
  routerToRightDevices = link.Install (routerNodes.Get (1), linuxNodes.Get (1));

  DceManagerHelper dceManager;
  LinuxStackHelper linuxStack;

  InternetStackHelper internetStack;

  if (stack == "linux")
    {
      sock_factory = "ns3::LinuxTcpSocketFactory";
      dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory",
                                  "Library", StringValue ("liblinux.so"));
      linuxStack.Install (linuxNodes);
      internetStack.Install (routerNodes);
    }
  else
    {
      internetStack.InstallAll ();
    }
  
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");

  Ipv4InterfaceContainer sink_interfaces;  

  Ipv4InterfaceContainer interfaces;
  address.Assign (leftToRouterDevices);
  address.NewNetwork ();
  address.Assign (bottleNeckDevices);
  address.NewNetwork ();
  interfaces = address.Assign (routerToRightDevices);
  sink_interfaces.Add (interfaces.Get (1));  

  if(stack == "linux")
    {
      dceManager.Install (linuxNodes);
      dceManager.Install (routerNodes);
      linuxStack.SysctlSet (linuxNodes, ".net.ipv4.conf.default.forwarding", "1");
      linuxStack.SysctlSet (linuxNodes, ".net.ipv4.tcp_congestion_control", transport_prot);
      linuxStack.SysctlSet (linuxNodes, ".net.ipv4.tcp_ecn", "1");
    }

  Ptr<Ipv4> ipv40 = routerNodes.Get (0)->GetObject<Ipv4> ();
  Ptr<Ipv4> ipv41 = routerNodes.Get (1)->GetObject<Ipv4> ();
  Ipv4StaticRoutingHelper routingHelper;
  Ptr<Ipv4StaticRouting> staticRouting0 = routingHelper.GetStaticRouting (ipv40);
  Ptr<Ipv4StaticRouting> staticRouting1 = routingHelper.GetStaticRouting (ipv41);
  staticRouting0->AddNetworkRouteTo (Ipv4Address ("10.0.2.0"), Ipv4Mask ("255.255.255.0"), Ipv4Address ("10.0.1.2"), 2);
  staticRouting1->AddNetworkRouteTo (Ipv4Address ("10.0.0.0"), Ipv4Mask ("255.255.255.0"), Ipv4Address ("10.0.1.1"), 1);

  LinuxStackHelper::RunIp (linuxNodes.Get (0), Seconds (0.1), "route add default via 10.0.0.1 dev sim0");
  LinuxStackHelper::RunIp (routerNodes.Get (0), Seconds (0.1), "route add default via 10.0.0.1 dev sim0");
  LinuxStackHelper::RunIp (routerNodes.Get (1), Seconds (0.1), "route add default via 10.0.2.1 dev sim0");
  LinuxStackHelper::RunIp (linuxNodes.Get (1), Seconds (0.1), "route add default via 10.0.2.1 dev sim0");
 
  dir += (currentTime + "/");
  std::string dirToSave = "mkdir -p " + dir;
  system (dirToSave.c_str ());
  system ((dirToSave + "/pcap/").c_str ());
  system ((dirToSave + "/queueTraces/").c_str ());

  TrafficControlHelper tch;
  tch.SetRootQueueDisc (queue_disc_type);
  QueueDiscContainer qd;

  Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (1500));
  // DCTCP tracks instantaneous queue length only; so set QW = 1
  Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (1));
  // Setting ECN is mandatory for DCTCP
  Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (17));
  Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (17));
  Config::SetDefault (queue_disc_type + "::MaxSize", QueueSizeValue (QueueSize ("100p")));

  tch.Uninstall (bottleNeckDevices.Get (0));

  qd = tch.Install (bottleNeckDevices.Get (0));
  Simulator::ScheduleNow (&qlen, qd.Get (0));

  uint16_t port = 50000;
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));

  PacketSinkHelper sinkHelper (sock_factory, sinkLocalAddress);
  
  AddressValue remoteAddress (InetSocketAddress (sink_interfaces.GetAddress (0, 0), port));
  BulkSendHelper sender (sock_factory, Address ());
    
  sender.SetAttribute ("Remote", remoteAddress);
  ApplicationContainer sendApp = sender.Install (linuxNodes.Get (0));
  sendApp.Start (Seconds (start_time));
  sendApp.Stop (Seconds (stop_time));

  ApplicationContainer sinkApp = sinkHelper.Install (linuxNodes.Get (1));
  sinkApp.Start (Seconds (start_time));
  sinkApp.Stop (Seconds (stop_time));


  if (pcap)
    {
       std::cout << "Pcap Enabled" << std::endl;
       link.EnablePcapAll (dir + "pcap/cwnd-trace-full-dumbbell-N", true);
       bottleNeckLink.EnablePcapAll (dir + "pcap/cwnd-trace-full-dumbbell-N", true);
    }

  for ( float i = start_time; i < stop_time ; i=i+0.1)
   {
     GetSSStats(linuxNodes.Get (0), Seconds(i), stack);
   }

  Simulator::Stop (Seconds (stop_time));
  Simulator::Run ();

  QueueDisc::Stats st = qd.Get (0)->GetStats ();
  std::cout << st << std::endl;

  std::ofstream myfile;
  myfile.open (dir + "config.txt", std::fstream::in | std::fstream::out | std::fstream::app);
  myfile << "useEcn " << true << "\n";
  myfile << "queue_disc_type " << queue_disc_type << "\n";
  myfile << "transport_prot " << transport_prot << "\n";
  myfile << "stopTime " << stop_time << "\n";
  myfile.close ();

  Simulator::Destroy ();
  return 0;
}

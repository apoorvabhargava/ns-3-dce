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

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("DceTrace");

//NodeContainer linuxNodes;

int main (int argc, char *argv[])
{
  uint32_t    nLeaf = 1;
  bool pcap = true;
  std::string stack = "linux";
  unsigned int num_flows = 3;
  std::string sock_factory = "ns3::TcpSocketFactory";
  float duration = 30;


  CommandLine cmd;
  cmd.AddValue ("nLeaf",     "Number of left and right side leaf nodes", nLeaf);
  cmd.AddValue ("pcap", "Enable PCAP", pcap);
  cmd.AddValue ("stack", "Network stack: either ns3 or Linux", stack);
  cmd.Parse (argc, argv);

  // Set the simulation start and stop time
  float start_time = 10.1;
  float stop_time = start_time + duration;

  if (stack != "ns3" && stack != "linux")
    {
      std::cout << "ERROR: " <<  stack << " is not available" << std::endl; 
    }

  //Ptr<Node> client = CreateObject<Node> ();
  //Ptr<Node> server = CreateObject<Node> ();
  NodeContainer nodes, routerNodes, linuxNodes;
  //routerNodes.Create (2);
  nodes.Create (4);
  linuxNodes.Add (nodes.Get (0));
  linuxNodes.Add (nodes.Get (3));
  routerNodes.Add (nodes.Get (1));
  routerNodes.Add (nodes.Get (2));
 

  PointToPointHelper link;
  link.SetDeviceAttribute ("DataRate", StringValue ("2Mbps"));
  link.SetChannelAttribute ("Delay", StringValue ("0.01ms"));

  NetDeviceContainer leftToRouterLeaf, routerToRightLeaf, bottleNeckLink;
  leftToRouterLeaf = link.Install (linuxNodes.Get (0), routerNodes.Get (0));
  bottleNeckLink = link.Install (routerNodes.Get (0), routerNodes.Get (1));
  routerToRightLeaf = link.Install (routerNodes.Get (1), linuxNodes.Get (1));

  DceManagerHelper dceManager;
  LinuxStackHelper linuxStack;

  InternetStackHelper internetStack;

  if (stack == "linux")
    {
      sock_factory = "ns3::LinuxTcpSocketFactory";
      //dceManager.SetTaskManagerAttribute ("FiberManagerType",
      //                                StringValue ("UcontextFiberManager"));
      dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory",
                                  "Library", StringValue ("liblinux.so"));
      linuxStack.Install (linuxNodes);
      internetStack.Install (routerNodes);
      //linuxStack.Install (routerNodes);
    }
  else
    {
      internetStack.InstallAll ();
    }
  
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");

  Ipv4InterfaceContainer sink_interfaces;  

  Ipv4InterfaceContainer interfaces;
  address.Assign (leftToRouterLeaf);
  address.NewNetwork ();
  address.Assign (bottleNeckLink);
  address.NewNetwork ();
  interfaces = address.Assign (routerToRightLeaf);
  sink_interfaces.Add (interfaces.Get (1));  

  std::cout << "About to install dce manager." << std::endl;
  
  std::ostringstream serverIp;
  Ptr<Ipv4> routeraddress = routerNodes.Get (0)->GetObject<Ipv4> ();
  Ipv4Address serverAddress = routeraddress->GetAddress (2, 0).GetLocal ();
  serverAddress.Print (serverIp);
  std::cout << "Ip Address:" << serverIp.str() << std::endl;

//  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  if(stack == "linux")
    {
      dceManager.Install (linuxNodes);
      dceManager.Install (routerNodes);
      linuxStack.SysctlSet (linuxNodes, ".net.ipv4.conf.default.forwarding", "1");
      linuxStack.SysctlSet (linuxNodes, ".net.ipv4.tcp_congestion_control", "dctcp");
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
 // LinuxStackHelper::PopulateRoutingTables ();
  

  Ipv4GlobalRoutingHelper g;
   Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>("dynamic-global-routing.routes_v2", std::ios::out);
   //g.PrintRoutingTable (routingStream, Seconds(12));
   g.PrintRoutingTableAllAt (Seconds (20), routingStream);
   //g.PrintRoutingTableAt (Seconds (12.0), routerNode.Get (0), routingStream);
 
  std::cout << "Dce Manager installed." << std::endl;

  std::cout << "Ip Address obtained." << std::endl;

  uint16_t port = 2000;
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  //Address sinkLocalAddress (InetSocketAddress (ipv4Server->GetAddress (0,0).GetLocal (), port));

  PacketSinkHelper sinkHelper (sock_factory, sinkLocalAddress);
  
  AddressValue remoteAddress (InetSocketAddress (sink_interfaces.GetAddress (0, 0), port));
  BulkSendHelper sender (sock_factory, Address ());
    

  sender.SetAttribute ("Remote", remoteAddress);
  ApplicationContainer sendApp = sender.Install (linuxNodes.Get (0));
  sendApp.Start (Seconds (start_time + 0.1));
  sendApp.Stop (Seconds (stop_time));

  ApplicationContainer sinkApp = sinkHelper.Install (linuxNodes.Get (1));
  sinkApp.Start (Seconds (start_time));
  sinkApp.Stop (Seconds (stop_time));


  if (pcap)
    {
       std::cout << "Pcap Enabled" << std::endl;
       link.EnablePcapAll ("cwnd-trace-full-dumbbell", true);
    }

  Simulator::Stop (Seconds (stop_time));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

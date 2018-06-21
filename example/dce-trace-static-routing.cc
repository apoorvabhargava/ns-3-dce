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

NodeContainer linuxNodes, routerNode;

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

static void Forwarding(std::string string1, std::string string2)
{
  std::cout << "String 1:" << string1 << std::endl;
  std::cout << "String 2:" << string2 << std::endl;
}

void TestStuff()
{
 LinuxStackHelper::SysctlGet (linuxNodes.Get (1), Simulator::Now (), ".net.ipv4.conf.default.forwarding", &Forwarding);
  LinuxStackHelper::SysctlGet (linuxNodes.Get (1), Simulator::Now (), ".net.ipv4.tcp_congestion_control", &Forwarding);
}


int main (int argc, char *argv[])
{
  bool pcap = true;
  std::string stack = "linux";
  unsigned int num_flows = 3;
  std::string sock_factory = "ns3::TcpSocketFactory";
  float duration = 30;


  CommandLine cmd;
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

  Ptr<Node> router = CreateObject<Node> ();
  Ptr<Node> client = CreateObject<Node> ();
  Ptr<Node> server = CreateObject<Node> ();
  routerNode = NodeContainer (router);
  linuxNodes = NodeContainer (client, server);

  PointToPointHelper link;
  link.SetDeviceAttribute ("DataRate", StringValue ("2Mbps"));
  link.SetChannelAttribute ("Delay", StringValue ("0.01ms"));

  NetDeviceContainer leftToRouterDevices, routerToRightDevices;
  leftToRouterDevices = link.Install (client, router);
  routerToRightDevices = link.Install (router, server);

  DceManagerHelper dceManager;
  LinuxStackHelper linuxStack;

  InternetStackHelper internetStack;

  if (stack == "linux")
    {
      sock_factory = "ns3::LinuxTcpSocketFactory";
      dceManager.SetTaskManagerAttribute ("FiberManagerType",
                                      StringValue ("UcontextFiberManager"));
      dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory",
                                  "Library", StringValue ("liblinux.so"));
      linuxStack.Install (linuxNodes);
      internetStack.Install (routerNode);
    }
  else
    {
      internetStack.InstallAll ();
    }
  
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");

  Ipv4InterfaceContainer sink_interfaces;  

  Ipv4InterfaceContainer interfaces;
  address.NewNetwork ();
  interfaces = address.Assign (leftToRouterDevices);
  address.NewNetwork ();
  interfaces = address.Assign (routerToRightDevices);
  sink_interfaces.Add (interfaces.Get (1));  
  
  std::ostringstream serverIp;
  Ptr<Ipv4> routeraddress = router->GetObject<Ipv4> ();
  Ipv4Address serverAddress = routeraddress->GetAddress (0, 0).GetLocal ();
  serverAddress.Print (serverIp);
  std::cout << "Ip Address:" << serverIp.str() << std::endl;

  Simulator::Schedule(Seconds(5), &TestStuff);


  if(stack == "linux")
    {
      dceManager.Install (routerNode);
      dceManager.Install (linuxNodes);
      linuxStack.SysctlSet (linuxNodes, ".net.ipv4.conf.default.forwarding", "1");
      linuxStack.SysctlSet (linuxNodes, ".net.ipv4.tcp_congestion_control", "dctcp");
      linuxStack.SysctlSet (linuxNodes, ".net.ipv4.tcp_ecn", "1");
    }

  LinuxStackHelper::RunIp (client, Seconds (0.01), "route add default via 10.0.1.2 dev sim0");
  LinuxStackHelper::RunIp (server, Seconds (0.01), "route add default via 10.0.2.1 dev sim0");

  Ipv4GlobalRoutingHelper g;
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>("routing_table", std::ios::out);
  g.PrintRoutingTableAllAt (Seconds (20), routingStream);

  uint16_t port = 50000;
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));

  PacketSinkHelper sinkHelper (sock_factory, sinkLocalAddress);
  
  AddressValue remoteAddress (InetSocketAddress (sink_interfaces.GetAddress (0, 0), port));
  BulkSendHelper sender (sock_factory, Address ());
    

  sender.SetAttribute ("Remote", remoteAddress);
  ApplicationContainer sendApp = sender.Install (client);
  sendApp.Start (Seconds (start_time));
  sendApp.Stop (Seconds (stop_time));

  ApplicationContainer sinkApp = sinkHelper.Install (server);
  sinkApp.Start (Seconds (start_time));
  sinkApp.Stop (Seconds (stop_time));

  if (pcap)
    {
       std::cout << "Pcap Enabled" << std::endl;
       link.EnablePcapAll ("cwnd-trace", true);
    }

  for ( float i = start_time; i < stop_time ; i++)
   {
     GetSSStats(linuxNodes.Get (0), Seconds(i), stack);
   }

  Simulator::Stop (Seconds (stop_time));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

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
 * Author: Sourabh Jain <sourabhjain560@outlook.com>
 *         Apoorva Bhargava <apoorvabhargava13@gmail.com>
 *         Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */
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

static Ptr<OutputStreamWrapper> cWndStream;

static void
tracer (uint32_t oldval, uint32_t newval)
{
  std::cout << "Oldvalue: " << oldval << " Newvalue: "<< newval << std::endl;
  *cWndStream->GetStream () << Simulator::Now ().GetSeconds () << "," << oldval << "," << newval << std::endl;
}

static void
cwnd (std::string file_name)
{
  AsciiTraceHelper ascii;
  cWndStream = ascii.CreateFileStream (file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&tracer));
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
  bool trace = true;
  bool pcap = false;
  std::string trace_file = "cwnd_trace_file.trace";
  std::string stack = "linux";
  unsigned int num_flows = 3;
  std::string sock_factory = "ns3::TcpSocketFactory";
  float duration = 100;

  CommandLine cmd;
  cmd.AddValue ("trace", "Enable trace", trace);
  cmd.AddValue ("pcap", "Enable PCAP", pcap);
  cmd.AddValue ("duration", "Simulation Duration", duration);
  cmd.AddValue ("stack", "Network stack: either ns3 or Linux", stack);
  cmd.Parse (argc, argv);

  // Set the simulation start and stop time
  float start_time = 0.1;
  float stop_time = start_time + duration;

  if (stack != "ns3" && stack != "linux")
    {
      std::cout << "ERROR: " <<  stack << " is not available" << std::endl; 
    }

  NodeContainer nodes;
  nodes.Create (3);

  PointToPointHelper link;
  link.SetDeviceAttribute ("DataRate", StringValue ("2Mbps"));
  link.SetChannelAttribute ("Delay", StringValue ("5ms"));

  NetDeviceContainer rightToRouterDevices, routerToLeftDevices;
  rightToRouterDevices = link.Install (nodes.Get (0), nodes.Get (1));
  routerToLeftDevices = link.Install (nodes.Get (1), nodes.Get (2));

  DceManagerHelper dceManager;
  LinuxStackHelper linuxStack;

  InternetStackHelper internetStack;

  if (stack == "linux")
    {

      sock_factory = "ns3::LinuxTcpSocketFactory";
      dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory",
                                  "Library", StringValue ("liblinux.so"));
      linuxStack.Install (nodes);
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
  interfaces = address.Assign (rightToRouterDevices);
  address.NewNetwork ();
  interfaces = address.Assign (routerToLeftDevices);
  sink_interfaces.Add (interfaces.Get (1));  

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  if(stack == "linux")
    {
      LinuxStackHelper::PopulateRoutingTables ();
      dceManager.Install (nodes);
      linuxStack.SysctlSet (nodes.Get (0), ".net.ipv4.tcp_congestion_control", "dctcp");
      linuxStack.SysctlSet (nodes.Get (2), ".net.ipv4.tcp_congestion_control", "dctcp");
      
      linuxStack.SysctlSet (nodes,   ".net.ipv4.conf.default.forwarding", "1"); 
      linuxStack.SysctlSet (nodes, ".net.ipv4.tcp_window_scaling", "1");
      linuxStack.SysctlSet (nodes, ".net.ipv4.tcp_dsack", "0");
      linuxStack.SysctlSet (nodes, ".net.ipv4.tcp_fack", "0");
      linuxStack.SysctlSet (nodes, ".net.ipv4.tcp_ecn", "1");
    }

  uint16_t port = 2000;
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));

  PacketSinkHelper sinkHelper (sock_factory, sinkLocalAddress);
  
  AddressValue remoteAddress (InetSocketAddress (sink_interfaces.GetAddress (0, 0), port));
  BulkSendHelper sender (sock_factory, Address ());
    
  sender.SetAttribute ("Remote", remoteAddress);
  ApplicationContainer sendApp = sender.Install (nodes.Get (0));
  sendApp.Start (Seconds (start_time));
  sendApp.Stop (Seconds (stop_time));

  ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get (2));
  sinkApp.Start (Seconds (start_time));
  sinkApp.Stop (Seconds (stop_time));

  if (trace)
    {
      Simulator::Schedule (Seconds (0.00001), &cwnd, trace_file);
    }

  if (pcap)
    {
       std::cout << "Pcap Enabled" << std::endl;
       link.EnablePcapAll ("cwnd-trace", true);
    }

  for ( float i = start_time; i < stop_time ; i=i+0.1)
   {
     GetSSStats(nodes.Get (0), Seconds(i), stack);
   }

  Simulator::Stop (Seconds (stop_time));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

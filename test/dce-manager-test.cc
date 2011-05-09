#include "ns3/test.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "dce-manager.h"
#include "task-manager.h"
#include "rr-task-scheduler.h"
#include "cooja-loader-factory.h"
#include "ns3/socket-fd-factory.h"
#include "ns3/ipv4.h"
#include "ns3/packet-socket-factory.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-static-routing-helper.h"

static std::string g_testError;

extern "C" void dce_manager_test_store_test_error (const char *s)
{
  g_testError = s;
}


namespace ns3 {

class DceManagerTestCase : public TestCase
{
public:
  DceManagerTestCase (std::string filename, Time maxDuration);
private:
  virtual void DoRun (void);
  Ptr<DceManager> CreateManager (int *pstatus);
  void StartApplication (Ptr<DceManager> manager, int *pstatus);
  void CreateAndAggregateObjectFromTypeId (Ptr<Node> node, const std::string typeId);
  void SetupSimpleStack (Ptr<Node> node);
  static void Finished (int *pstatus, uint16_t pid, int status);

  std::string m_filename;
  ObjectFactory m_networkStackFactory;
  ObjectFactory m_tcpFactory;
  const Ipv4RoutingHelper *m_routing;
  Time m_maxDuration;
};

DceManagerTestCase::DceManagerTestCase (std::string filename, Time maxDuration)
  : TestCase ("Check that process \"" + filename + "\" completes correctly."),
    m_filename (filename), m_maxDuration ( maxDuration )
{

}
void
DceManagerTestCase::StartApplication (Ptr<DceManager> manager, int *pstatus)
{
  std::vector<std::string> noargs;
  std::vector<std::pair<std::string,std::string> > noenv;
  
  uint16_t pid = manager->Start (m_filename, 1<<20, noargs, noenv);
  manager->SetFinishedCallback (pid, MakeBoundCallback (&DceManagerTestCase::Finished, pstatus));
}
void
DceManagerTestCase::CreateAndAggregateObjectFromTypeId (Ptr<Node> node, const std::string typeId)
{
  ObjectFactory factory;
  factory.SetTypeId (typeId);
  Ptr<Object> protocol = factory.Create <Object> ();
  node->AggregateObject (protocol);
}
void
DceManagerTestCase::SetupSimpleStack (Ptr<Node> node)
{
  m_networkStackFactory.SetTypeId ("ns3::Ns3SocketFdFactory");
  m_tcpFactory.SetTypeId ("ns3::TcpL4Protocol");
  Ipv4StaticRoutingHelper staticRouting;
  Ipv4ListRoutingHelper listRouting;
  listRouting.Add (staticRouting, 0);
  m_routing = listRouting.Copy ();

  if (node->GetObject<Ipv4> () != 0)
    {
      NS_FATAL_ERROR ("InternetStackHelper::Install (): Aggregating "
                      "an InternetStack to a node with an existing Ipv4 object");
    }

  CreateAndAggregateObjectFromTypeId (node, "ns3::ArpL3Protocol");
  CreateAndAggregateObjectFromTypeId (node, "ns3::Ipv4L3Protocol");
  CreateAndAggregateObjectFromTypeId (node, "ns3::Icmpv4L4Protocol");
  CreateAndAggregateObjectFromTypeId (node, "ns3::UdpL4Protocol");
  node->AggregateObject (m_tcpFactory.Create<Object> ());
  Ptr<PacketSocketFactory> factory = CreateObject<PacketSocketFactory> ();
  node->AggregateObject (factory);
  // Set routing
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  Ptr<Ipv4RoutingProtocol> ipv4Routing = m_routing->Create (node);
  ipv4->SetRoutingProtocol (ipv4Routing);

  Ptr<SocketFdFactory> networkStack = m_networkStackFactory.Create<SocketFdFactory> ();
  NS_ASSERT( 0 != networkStack );

  node->AggregateObject (networkStack);

}
Ptr<DceManager>
DceManagerTestCase::CreateManager (int *pstatus)
{
  Ptr<Node> a = CreateObject<Node> ();
  Ptr<TaskManager> taskManager = CreateObject<TaskManager> ();
  Ptr<TaskScheduler> taskScheduler = CreateObject<RrTaskScheduler> ();
  Ptr<DceManager> aManager = CreateObject<DceManager> ();
  Ptr<LoaderFactory> loaderFactory = CreateObject<CoojaLoaderFactory> ();


  taskManager->SetScheduler (taskScheduler);
  a->AggregateObject (loaderFactory);
  a->AggregateObject (taskManager);
  a->AggregateObject (aManager);

  SetupSimpleStack (a);

  Simulator::ScheduleWithContext (a->GetId (), Seconds (0.0),
				  &DceManagerTestCase::StartApplication, this, 
				  aManager, pstatus);
  return aManager;
}
void
DceManagerTestCase::Finished (int *pstatus, uint16_t pid, int status)
{
  *pstatus = status;
}
void
DceManagerTestCase::DoRun (void)
{
  int status = - 1;
  Ptr<DceManager> a = CreateManager (&status);

  if (m_maxDuration.IsStrictlyPositive()) {
      Simulator::Stop ( m_maxDuration );
  }
  Simulator::Run ();
  Simulator::Destroy ();
  NS_TEST_ASSERT_MSG_EQ (status, 0, "Process did not return successfully: " << g_testError);
  //  return status != 0;
}

static class DceManagerTestSuite : public TestSuite
{
public:
  DceManagerTestSuite ();
private:
} g_processTests;



DceManagerTestSuite::DceManagerTestSuite ()
  : TestSuite ("process-manager", UNIT)
{
  typedef struct {
    const char *name;
    int duration;
  } testPair;

  const testPair tests[] = {
    /*  { "test-empty", 0 },
      {  "test-sleep", 0 },
      {  "test-pthread", 0 },
      {  "test-mutex", 0 },
      {  "test-once", 0 },
      {  "test-pthread-key", 0 },
      {  "test-sem", 0 },
      {  "test-malloc", 0 },
      {  "test-malloc-2", 0 },
      {  "test-fd-simple", 0 },
      {  "test-strerror", 0 },
      {  "test-stdio", 0 },
      {  "test-string", 0 },
      {  "test-netdb", 0 },
      {  "test-env", 0 },
      {  "test-cond", 0 },
      {  "test-timer-fd", 0 },
      {  "test-stdlib", 0 }, */
      {  "test-select", 320 }, /*
      {  "test-nanosleep", 0 },
      {  "test-random", 0 },
      {  "test-fork", 0 },
      {  "test-local-socket", 0 },*/
  };
  for (unsigned int i = 0; i < sizeof(tests)/sizeof(testPair);i++)
    {
      AddTestCase (new DceManagerTestCase (tests[i].name ,  Seconds (tests[i].duration) ) );
    }
}

} // namespace ns3

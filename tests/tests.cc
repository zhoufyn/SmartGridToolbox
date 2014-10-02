#define BOOST_TEST_MODULE test_template

#include <LibSgtCore.h>
#include <LibSgtSim.h>

#include <boost/test/included/unit_test.hpp>

#include <cmath>
#include <ostream>
#include <fstream>

using namespace SmartGridToolbox;
using namespace std;
using namespace boost::posix_time;
using namespace boost::unit_test;

BOOST_AUTO_TEST_CASE (test_weak_order)
{
   WoGraph g(6);
   g.link(3, 1);
   g.link(4, 1);
   g.link(0, 4);
   g.link(1, 0);
   g.link(1, 2);
   g.link(0, 5);
   g.link(5, 2);
   // Order should be 3, (0, 1, 4), 5, 2.
   g.weakOrder();
   for (int i = 0; i < g.size(); ++i)
   {
      Log().message() << " " << g.nodes()[i]->index() << endl;
   }
   Log().message() << endl;

   Log().message() << "   ";
   for (int i = 0; i < g.size(); ++i)
   {
      std::cout << " " << g.nodes()[i]->index(); 
   }
   std::cout << endl << endl;
   
   for (const std::unique_ptr<WoNode> & nd1 : g.nodes())
   {
      Log().message() << nd1->index() << "   ";
      for (const std::unique_ptr<WoNode> & nd2 : g.nodes())
      {
         std::cout << nd1->dominates(*nd2) << " ";
      }
      std::cout << endl;
   }

   BOOST_CHECK(g.nodes()[0]->index() == 3);
   BOOST_CHECK(g.nodes()[1]->index() == 0);
   BOOST_CHECK(g.nodes()[2]->index() == 1);
   BOOST_CHECK(g.nodes()[3]->index() == 4);
   BOOST_CHECK(g.nodes()[4]->index() == 5);
   BOOST_CHECK(g.nodes()[5]->index() == 2);
}

BOOST_AUTO_TEST_CASE (test_overhead_compare_carson_1)
{
   // For this test, we compare values for an overhead line examined in one of Kersting's papers:
   // docs/background/Kersting_Carson.pdf.
   Network netw("network");
   Parser<Network> p;
   p.parse("test_overhead_compare_carson_1.yaml", netw);

   auto oh = std::dynamic_pointer_cast<OverheadLine>(netw.arc("line_1_2")->branch());

   ublas::matrix<Complex> ZWire = oh->ZWire();
   ublas::matrix<Complex> ZPhase = oh->ZPhase(ZWire);
   Log().message() << "ZWire = " << std::endl;
   for (int i = 0; i < ZWire.size1(); ++i)
   {
      Log().message() << row(ZWire, i) << std::endl;
   }
   Log().message() << "ZPhase = " << std::endl;
   for (int i = 0; i < ZPhase.size1(); ++i)
   {
      Log().message() << row(ZPhase, i) << std::endl;
   }
   Log().message() << "YNode = " << std::endl;
   for (int i = 0; i < oh->Y().size1(); ++i)
   {
      Log().message() << row(oh->Y(), i) << std::endl;
   }

   Complex cmp;
   cmp = {1.3369, 1.3331}; double err00 = abs(ZPhase(0,0) - cmp) / abs(cmp);
   cmp = {0.2102, 0.5778}; double err01 = abs(ZPhase(0, 1) - cmp) / abs(cmp);
   cmp = {0.2132, 0.5014}; double err02 = abs(ZPhase(0, 2) - cmp) / abs(cmp);
   cmp = {1.3239, 1.3557}; double err11 = abs(ZPhase(1, 1) - cmp) / abs(cmp);
   cmp = {0.2067, 0.4591}; double err12 = abs(ZPhase(1, 2) - cmp) / abs(cmp);
   cmp = {1.3295, 1.3459}; double err22 = abs(ZPhase(2, 2) - cmp) / abs(cmp);
   Log().message() << "Err = " 
             << err00 << " " << err01 << " " << err02 << " " << err11 << " " << err12 << " " << err22 << std::endl;

   BOOST_CHECK(err00 < 0.001);
   BOOST_CHECK(err01 < 0.001);
   BOOST_CHECK(err02 < 0.001);
   BOOST_CHECK(err11 < 0.001);
   BOOST_CHECK(err12 < 0.001);
   BOOST_CHECK(err22 < 0.001);

   BOOST_CHECK_EQUAL(ZPhase(0,1), ZPhase(1,0));
   BOOST_CHECK_EQUAL(ZPhase(0,2), ZPhase(2,0));
   BOOST_CHECK_EQUAL(ZPhase(1,2), ZPhase(2,1));
}

BOOST_AUTO_TEST_CASE (test_overhead_compare_carson_2)
{
   // For this test, we compare values for an overhead line examined in one of Kersting's papers:
   // docs/background/Kersting_Carson.pdf.
   Network netw("network");
   Parser<Network> p;
   p.parse("test_overhead_compare_carson_2.yaml", netw);

   netw.solvePowerFlow();

   auto oh = std::dynamic_pointer_cast<OverheadLine>(netw.arc("line_1_2")->branch());

   auto bus1 = netw.node("bus_1")->bus();
   auto bus2 = netw.node("bus_2")->bus();

   Log().message() << "Bus 1 voltages: " << abs(bus1->V()(0)) << "@" << arg(bus1->V()(0)) * 180 / pi << std::endl;
   Log().message() << "Bus 1 voltages: " << abs(bus1->V()(1)) << "@" << arg(bus1->V()(1)) * 180 / pi << std::endl;
   Log().message() << "Bus 1 voltages: " << abs(bus1->V()(2)) << "@" << arg(bus1->V()(2)) * 180 / pi << std::endl;
   Log().message() << "Bus 2 voltages: " << abs(bus2->V()(0)) << "@" << arg(bus2->V()(0)) * 180 / pi << std::endl;
   Log().message() << "Bus 2 voltages: " << abs(bus2->V()(1)) << "@" << arg(bus2->V()(1)) * 180 / pi << std::endl;
   Log().message() << "Bus 2 voltages: " << abs(bus2->V()(2)) << "@" << arg(bus2->V()(2)) * 180 / pi << std::endl;

   Complex cmp;
   cmp = polar(14.60660, -0.62 * pi / 180.0); double err0 = abs(bus2->V()(0) - cmp) / abs(cmp);
   cmp = polar(14.72669, -121.0 * pi / 180.0); double err1 = abs(bus2->V()(1) - cmp) / abs(cmp);
   cmp = polar(14.80137, 119.2 * pi / 180.0); double err2 = abs(bus2->V()(2) - cmp) / abs(cmp);
   Log().message() << "Err = " << err0 << " " << err1 << " " << err2 << std::endl;

   BOOST_CHECK(err0 < 0.001);
   BOOST_CHECK(err1 < 0.001);
   BOOST_CHECK(err2 < 0.001);
}

#if 0
BOOST_AUTO_TEST_CASE (test_dependencies)
{
   Simulation sim;

   SimBus & a0 = sim.newComponent<SimBus>("bus0", 0, 0.1);
   SimBus & a1 = sim.newComponent<SimBus>("bus1", 1, 0.1);
   SimBus & a2 = sim.newComponent<SimBus>("bus2", 2, 0.1);
   SimBus & a3 = sim.newComponent<SimBus>("bus3", 3, 0.1);
   SimBus & a4 = sim.newComponent<SimBus>("bus4", 4, 0.1);
   SimBus & a5 = sim.newComponent<SimBus>("bus5", 5, 0.1);

   a4.dependsOn(a0);
   a5.dependsOn(a0);
   a0.dependsOn(a1);
   a2.dependsOn(a1);
   a1.dependsOn(a3);
   a1.dependsOn(a4);
   a2.dependsOn(a5);

   mod.validate();

   BOOST_CHECK(mod.components()[0] == &a3);
   BOOST_CHECK(mod.components()[1] == &a0);
   BOOST_CHECK(mod.components()[2] == &a1);
   BOOST_CHECK(mod.components()[3] == &a4);
   BOOST_CHECK(mod.components()[4] == &a5);
   BOOST_CHECK(mod.components()[5] == &a2);
}

BOOST_AUTO_TEST_CASE (test_simple_battery)
{
   local_time::time_zone_ptr tz(new local_time::posix_time_zone("UTC0"));
   using namespace boost::gregorian;
   SimpleBattery bat1("sb0");
   bat1.setName("bat1");
   bat1.setInitCharge(5.0 * kWh);
   bat1.setMaxCharge(8.0 * kWh);
   bat1.setMaxChargePower(1.0 * kW);
   bat1.setMaxDischargePower(2.1 * kW);
   bat1.setChargeEfficiency(0.9);
   bat1.setDischargeEfficiency(0.8);
   bat1.setRequestedPower(-0.4 * kW);
   bat1.initialize();
   bat1.update(timeFromLocalTime(posix_time::ptime(gregorian::date(2012, Feb, 11), posix_time::hours(2)), tz));
   Log().message() << "1 Battery charge = " << bat1.charge() / kWh << endl;
   bat1.update(bat1.time() + posix_time::hours(3));
   Log().message() << "2 Battery charge = " << bat1.charge() / kWh << endl;
   double comp = bat1.initCharge() + 
      dSeconds(posix_time::hours(3)) * bat1.requestedPower() /
      bat1.dischargeEfficiency();
   Log().message() << "comp = " << comp / kWh << endl;
   BOOST_CHECK(bat1.charge() == comp);

   bat1.setRequestedPower(1.3 * kW);
   bat1.initialize();
   bat1.update(timeFromLocalTime(posix_time::ptime(gregorian::date(2012, Feb, 11), posix_time::hours(2)), tz));
   Log().message() << "3 Battery charge = " << bat1.charge() / kWh << endl;
   bat1.update(bat1.time() + posix_time::hours(3));
   Log().message() << "4 Battery charge = " << bat1.charge() / kWh << endl;
   comp = bat1.initCharge() + 
      dSeconds(posix_time::hours(3)) * bat1.maxChargePower() *
      bat1.chargeEfficiency();
   Log().message() << "comp = " << comp / kWh << endl;
   BOOST_CHECK(bat1.charge() == comp);

   bat1.setRequestedPower(-1 * kW);
   bat1.initialize();
   bat1.update(timeFromLocalTime(posix_time::ptime(gregorian::date(2012, Feb, 11), posix_time::hours(2)), tz));
   Log().message() << "3 Battery charge = " << bat1.charge() / kWh << endl;
   bat1.update(bat1.time() + posix_time::hours(5) + posix_time::minutes(30));
   Log().message() << "4 Battery charge = " << bat1.charge() / kWh << endl;
   comp = 0.0;
   Log().message() << "comp = " << comp / kWh << endl;
   BOOST_CHECK(bat1.charge() == comp);
}

BOOST_AUTO_TEST_CASE (test_spline)
{
  Spline spline;
  //Add points to the spline in any order, they're sorted in ascending
  //x later. (If you want to spline a circle you'll need to change the class)
  spline.addPoint(0,        0.0);
  spline.addPoint(40.0 / 255, 0.0);
  spline.addPoint(60.0 / 255, 0.2);
  spline.addPoint(63.0 / 255, 0.05);
  spline.addPoint(80.0 / 255, 0.0);
  spline.addPoint(82.0 / 255, 0.9);
  spline.addPoint(1.0, 1.0);

  { //We can extract the original data points by treating the spline as
    //a read-only STL container.
    std::ofstream of("orig.dat");
    for (Spline::const_iterator iPtr = spline.begin(); iPtr != spline.end(); ++iPtr)
      of << iPtr->first << " " << iPtr->second << "\n";
  }
  
  { //A "natural spline" where the second derivatives are set to 0 at the boundaries.

    //Each boundary condition is set seperately

    //The following aren't needed as its the default setting. The 
    //zero values are the second derivatives at the spline points.
    spline.setLowBC(Spline::FIXED_2ND_DERIV_BC, 0);
    spline.setHighBC(Spline::FIXED_2ND_DERIV_BC, 0);

    //Note: We can calculate values outside the range spanned by the
    //points. The extrapolation is based on the boundary conditions
    //used.

    std::ofstream of("spline.natural.dat");
    for (double x(-0.2); x <= 1.2001; x += 0.005)
      of << x << " " << spline(x) << "\n";
  }
}   

BOOST_AUTO_TEST_CASE (test_spline_timeseries)
{
   local_time::time_zone_ptr tz(new local_time::posix_time_zone("UTC0"));
   using namespace boost::gregorian;
   Time base = timeFromLocalTime(posix_time::ptime(gregorian::date(2013, Apr, 26), posix_time::hours(0)), tz);
   SplineTimeSeries<Time> sts;
   sts.addPoint(base + posix_time::hours(0), sin(0 * pi / 12));
   sts.addPoint(base + posix_time::hours(4), sin(4 * pi / 12));
   sts.addPoint(base + posix_time::hours(8), sin(8 * pi / 12));
   sts.addPoint(base + posix_time::hours(12), sin(12 * pi / 12));
   sts.addPoint(base + posix_time::hours(16), sin(16 * pi / 12));
   sts.addPoint(base + posix_time::hours(20), sin(20 * pi / 12));
   sts.addPoint(base + posix_time::hours(24), sin(24 * pi / 12));
   for (int i = -1; i <= 25; ++i)
   {
      double val = sts.value(base + posix_time::hours(i));
      double err = std::abs(val - sin(i * pi / 12));
      Log().message() << i << " " << val << " " << err << endl; 
      if (i > -1 && i < 25)
      {
         BOOST_CHECK(err < 0.005);
      }
   }
}

BOOST_AUTO_TEST_CASE (test_lerp_timeseries)
{
   local_time::time_zone_ptr tz(new local_time::posix_time_zone("UTC0"));
   using namespace boost::gregorian;
   Time base = timeFromLocalTime(posix_time::ptime(gregorian::date(2013, Apr, 26), posix_time::hours(0)), tz);
   LerpTimeSeries<Time, Complex> lts;
   lts.addPoint(base + posix_time::hours(0), Complex(0, 0));
   lts.addPoint(base + posix_time::hours(1), Complex(3, 1));
   lts.addPoint(base + posix_time::hours(3), Complex(10, 11));
   for (int i = -1; i <= 4; ++i)
   {
      Complex val = lts.value(base + posix_time::hours(i));
      Log().message() << i << " " << val << endl; 
   }
   BOOST_CHECK(lts.value(base + posix_time::hours(-1)) == Complex(0, 0));
   BOOST_CHECK(lts.value(base + posix_time::hours(0)) == Complex(0, 0));
   BOOST_CHECK(lts.value(base + posix_time::minutes(30)) == Complex(1.5, 0.5));
   BOOST_CHECK(lts.value(base + posix_time::hours(1)) == Complex(3, 1));
   BOOST_CHECK(lts.value(base + posix_time::hours(2)) == Complex(6.5, 6));
   BOOST_CHECK(lts.value(base + posix_time::hours(3)) == Complex(10, 11));
   BOOST_CHECK(lts.value(base + posix_time::hours(4) + posix_time::seconds(1)) == Complex(10, 11));
}

BOOST_AUTO_TEST_CASE (test_stepwise_timeseries)
{
   time_duration base(posix_time::minutes(5));
   StepwiseTimeSeries<time_duration, double> sts;
   sts.addPoint(base + posix_time::hours(0), 1.5);
   sts.addPoint(base + posix_time::hours(1), 2.5);
   sts.addPoint(base + posix_time::hours(3), 5.5);
   for (int i = -1; i <= 4; ++i)
   {
      double val = sts.value(base + posix_time::hours(i));
      Log().message() << i << " " << val << endl; 
   }
   Log().message() << endl;
   for (int i = -1; i <= 4; ++i)
   {
      double val = sts.value(base + posix_time::hours(i) + posix_time::seconds(1));
      Log().message() << i << " " << val << endl; 
   }
   BOOST_CHECK(sts.value(base + posix_time::seconds(-1)) == 1.5);
   BOOST_CHECK(sts.value(base + posix_time::seconds(1)) == 1.5);
   BOOST_CHECK(sts.value(base + posix_time::hours(1) - posix_time::seconds(1)) == 1.5);
   BOOST_CHECK(sts.value(base + posix_time::hours(1) + posix_time::seconds(1)) == 2.5);
   BOOST_CHECK(sts.value(base + posix_time::hours(3) + posix_time::seconds(1)) == 5.5);
}

BOOST_AUTO_TEST_CASE (test_function_timeseries)
{
   FunctionTimeSeries <time_duration, double> 
      fts([] (time_duration td) {return 2 * dSeconds(td);});
   Log().message() << fts.value(posix_time::seconds(10)+milliseconds(3)) << endl;
   BOOST_CHECK(fts.value(posix_time::seconds(-1)) == -2.0);
   BOOST_CHECK(fts.value(posix_time::seconds(3)) == 6.0);
}

enum class Event : int
{
   ZERO,
   ONE,
   TWO
};

class TestEventA : public Component
{
   public:

      TestEventA(const std::string & name, time_duration dt, int ctrl) : 
         Component(name),
         state_(0),
         dt_(dt),
         ctrl_(ctrl)
      {
      }

      virtual time_duration validUntil() const override
      {
         return nextUpdate_;
      }

   private:
      // Reset state of the object, time is at timestamp t_.
      virtual void initializeState() override
      {
         startTime_ = time();
         nextUpdate_ = time() + dt_; 
      }

      // Bring state up to time t_.
      virtual void updateState(Time t) override
      {
         Log().message() << "Update state of " << name() << " from time " 
              << time() << " to " << t << "." << endl;
         state_ = (t - startTime_).ticks() * ctrl_;
         nextUpdate_ = t + dt_;
      }

   private:
      Time nextUpdate_;
      Time startTime_;
      int state_;
      time_duration dt_;
      int ctrl_;
};

BOOST_AUTO_TEST_CASE (test_events_and_sync)
{
   Model mod;
   TestEventA & a0 = mod.newComponent<TestEventA>("a0", posix_time::seconds(3), 3);
   TestEventA & a1 = mod.newComponent<TestEventA>("a1", posix_time::seconds(9), 3);
   a0.dependsOn(a1); 
   mod.validate();
   Simulation sim(mod);
   sim.setStartTime(posix_time::seconds(0));
   sim.setEndTime(posix_time::seconds(10));
   sim.initialize();

   a0.didCompleteTimestep().addAction(
         [&]() {Log().message() << a1.name() << " received did update from " << a0.name() << std::endl;},
         "Test action.");

   sim.doNextUpdate();
   sim.doNextUpdate();
   sim.doNextUpdate();
   sim.doNextUpdate();
   sim.doNextUpdate();
   sim.doNextUpdate();
   sim.doNextUpdate();
   sim.doNextUpdate();
}

static double sinusoidal(double t, double T, double Delta, 
                         double minim, double maxim)
{
   // Sinusoidal function, T is period, Delta is offset.
   return minim + (maxim - minim) * 0.5 * (1.0 + cos(2.0 * pi * (t - Delta) / T));
}

BOOST_AUTO_TEST_CASE (test_sparse_solver)
{
   int n = 5;
   ublas::compressed_matrix<double> a(n, n);
   a(0, 0) = 2.0;
   a(0, 1) = 3.0;
   a(1, 0) = 3.0;
   a(1, 2) = 4.0;
   a(1, 4) = 6.0;
   a(2, 1) = -1.0;
   a(2, 2) = -3.0;
   a(2, 3) = 2.0;
   a(3, 2) = 1.0;
   a(4, 1) = 4.0;
   a(4, 2) = 2.0;
   a(4, 4) = 1.0;

   ublas::vector<double> b(n);
   b(0) = 8.0;
   b(1) = 45.0;
   b(2) = -3.0;
   b(3) = 3.0;
   b(4) = 19.0;

   ublas::vector<double> x(n);
   kluSolve(a, b, x);
   Log().message(); for (int i = 0; i < n; ++i) std::cout << x(i) << " ";
   std::cout << endl;
   BOOST_CHECK(std::abs(x(0) - 1.0) < 1e-4);
   BOOST_CHECK(std::abs(x(1) - 2.0) < 1e-4);
   BOOST_CHECK(std::abs(x(2) - 3.0) < 1e-4);
   BOOST_CHECK(std::abs(x(3) - 4.0) < 1e-4);
   BOOST_CHECK(std::abs(x(4) - 5.0) < 1e-4);
}

inline Array2D<Complex, 2, 2> lineY(Complex y)
{
   return {{{y, -y},{-y, y}}};
}


class TestLoad : public ZipToGroundBase
{
   public:
      TestLoad(const std::string & name, Phases phases, Time dt) : ZipToGroundBase(name, phases), dt_(dt) {}

      virtual Time validUntil() const override
      {
         return time() + dt_;
      }

      virtual ublas::vector<Complex> Y() const override 
      {
         return ublas::vector<Complex>(phases().size(), sin(dSeconds(time()) / 123.0));
      }

      virtual ublas::vector<Complex> I() const override 
      {
         return ublas::vector<Complex>(phases().size(), 
               sin(dSeconds(time()) / 300.0) * exp(Complex(0.0, dSeconds(time()) / 713.0)));
      }

      virtual ublas::vector<Complex> S() const override 
      {
         return ublas::vector<Complex>(phases().size(), sin(dSeconds(time()) / 60.0));
      }

      time_duration dt() const
      {
         return dt_;
      }
      void setDt(time_duration dt)
      {
         dt_ = dt;
      }

   private:
      time_duration dt_;
};

BOOST_AUTO_TEST_CASE (test_phases)
{
   Phases p1 = Phase::A | Phase::B;
   BOOST_CHECK(p1.size() == 2);
   BOOST_CHECK(p1[0] == Phase::A);
   BOOST_CHECK(p1[1] == Phase::B);
   Phases p2{str2Phase("SP") | str2Phase("SM") | Phase::SN};
   for (auto pair : p2)
   {
      BOOST_CHECK((pair.second == 0 && pair.first == Phase::SP) ||
                  (pair.second == 1 && pair.first == Phase::SM) ||
                  (pair.second == 2 && pair.first == Phase::SN));
   }
   Phases p3 = p1 | p2;
   BOOST_CHECK(p3.hasPhase(Phase::A));
   BOOST_CHECK(p3.hasPhase(Phase::B));
   BOOST_CHECK(!p3.hasPhase(Phase::C));
   BOOST_CHECK(p3.hasPhase(Phase::SP));
   BOOST_CHECK(p3.hasPhase(Phase::SM));
   BOOST_CHECK(p3.hasPhase(Phase::SN));
   p3 &= Phase::A;
   BOOST_CHECK(p3 == Phase::A);
   Phases p4 = p1 | Phase::C;
   BOOST_CHECK(p1.isSubsetOf(p4));
}

BOOST_AUTO_TEST_CASE (test_network_1p)
{
   Model mod;
   Simulation sim(mod);

   Parser & p = Parser::globalParser();
   p.parse("test_network_1p.yaml", mod, sim); p.postParse();

   Bus * bus1 = mod.component<Bus>("bus_1");
   Bus * bus2 = mod.component<Bus>("bus_2");
   Bus * bus3 = mod.component<Bus>("bus_3");

   TestLoad & tl0 = mod.newComponent<TestLoad>("tl0", bus2->phases(), posix_time::seconds(5));
   bus2->addZipToGround(tl0);

   mod.validate();
   sim.initialize();

   Network * network = mod.component<Network>("network_1");
   ofstream outfile;
   outfile.open("test_network_1p.out");
   network->didCompleteTimestep().addAction([&]()
         {
            outfile << dSeconds(sim.currentTime()-sim.startTime()) << " " << bus1->V()(0) 
                    << " " << bus2->V()(0) << " " << bus3->V()(0) << std::endl;
         }, "Network updated.");

   while (sim.doNextUpdate())
   {
   }
   outfile.close();
}

BOOST_AUTO_TEST_CASE (test_network_b3p)
{
   Model mod;
   Simulation sim(mod);

   Parser & p = Parser::globalParser();
   p.parse("test_network_b3p.yaml", mod, sim); p.postParse();

   Bus * bus1 = mod.component<Bus>("bus_1");
   Bus * bus2 = mod.component<Bus>("bus_2");
   Bus * bus3 = mod.component<Bus>("bus_3");

   TestLoad & tl0 = mod.newComponent<TestLoad>("tl0", bus2->phases(), posix_time::seconds(5));
   bus2->addZipToGround(tl0);

   mod.validate();
   sim.initialize();

   Network * network = mod.component<Network>("network_1");
   ofstream outfile;
   outfile.open("test_network_b3p.out");
   outfile << dSeconds(sim.currentTime()-sim.startTime()) << " " 
           << bus1->V()(0) << " " << bus1->V()(1) << " " << bus1->V()(2)
           << bus2->V()(0) << " " << bus2->V()(1) << " " << bus2->V()(2)
           << bus3->V()(0) << " " << bus3->V()(1) << " " << bus3->V()(2)
           << std::endl;
   network->didCompleteTimestep().addAction([&]()
         {
            outfile << dSeconds(sim.currentTime()-sim.startTime()) << " " 
                    << bus1->V()(0) << " " << bus1->V()(1) << " " << bus1->V()(2) << " "
                    << bus2->V()(0) << " " << bus2->V()(1) << " " << bus2->V()(2) << " "
                    << bus3->V()(0) << " " << bus3->V()(1) << " " << bus3->V()(2)
                    << std::endl;
         }, "Network updated.");

   while (sim.doNextUpdate())
   {
   }
   outfile.close();
}

BOOST_AUTO_TEST_CASE (test_network_2p_identical)
{
   Model mod;
   Simulation sim(mod);

   Parser & p = Parser::globalParser();
   p.parse("test_network_2p_identical.yaml", mod, sim); p.postParse();

   Bus * bus1 = mod.component<Bus>("bus_1");
   Bus * bus2 = mod.component<Bus>("bus_2");
   Bus * bus3 = mod.component<Bus>("bus_3");

   TestLoad & tl0 = mod.newComponent<TestLoad>("tl0", bus2->phases(), posix_time::seconds(5));
   bus2->addZipToGround(tl0);

   mod.validate();
   sim.initialize();

   Network * network = mod.component<Network>("network_1");
   ofstream outfile;
   outfile.open("test_network_2p_identical.out");
   outfile << dSeconds(sim.currentTime()-sim.startTime()) << " " 
           << bus1->V()(0) << " " << bus2->V()(0) << " " << bus3->V()(0) << std::endl;
   network->didCompleteTimestep().addAction([&]()
         {
            outfile << dSeconds(sim.currentTime()-sim.startTime()) << " " 
                    << bus1->V()(0) << " " << bus2->V()(0) << " " << bus3->V()(0) << std::endl;
         }, "Network updated.");

   while (sim.doNextUpdate())
   {
   }
   outfile.close();
}

class TestDc : public DcPowerSourceBase
{
   public:
      TestDc(const std::string & name) : DcPowerSourceBase(name), dt_(posix_time::seconds(0)) {}

      virtual Time validUntil() const override
      {
         return time() + dt_;
      }

      time_duration dt() const
      {
         return dt_;
      }
      void setDt(time_duration dt)
      {
         dt_ = dt;
      }

      virtual double PDc() const override
      {
         return sin(dSeconds(time()) / 60.0);
      }

   private:
      time_duration dt_;
};

BOOST_AUTO_TEST_CASE (test_sun)
{
   // Canberra, Australia Lat Long = 35.3075 S, 149.1244 E (-35.3075, 149.1244).
   // Round off to (-35, 149) because reference calculator uses integer latlongs.
   // Take Jan 26 2013, 8:30 AM.
   // This is daylight savings time, UMT +11 hours.
   // http://pveducation.org/pvcdrom/properties-of-sunlight/sun-position-calculator is reference:
   // Values from website are: zenith: 64.47, azimuth: 96.12 (i.e. nearly due east, 0 is north, 90 is east).

   using namespace boost::gregorian;
   local_time::time_zone_ptr tz(new local_time::posix_time_zone("AEST10AEDT,M10.5.0/02,M3.5.0/03"));

   SphericalAngles sunCoords = sunPos(utcTimeFromLocalTime(posix_time::ptime(gregorian::date(2013, Jan, 26), posix_time::hours(8) + posix_time::minutes(30)), tz), 
         {-35.0, 149.0});
   Log().message() << "UTC time:  " << utcTimeFromLocalTime(posix_time::ptime(gregorian::date(2013, Jan, 26), posix_time::hours(8) + posix_time::minutes(30)), tz) 
             << std::endl;
   Log().message() << "Zenith:    " << sunCoords.zenith * 180 / pi << " expected: " << 64.47 << std::endl;
   Log().message() << "Azimuth:   " << sunCoords.azimuth * 180 / pi << " expected: " << 96.12 << std::endl;

   BOOST_CHECK(std::abs(sunCoords.zenith * 180 / pi - 64.47) < 1.25); // 5 minutes error.
   BOOST_CHECK(std::abs(sunCoords.azimuth * 180 / pi - 96.12) < 1.25); // 5 minutes error.

   Model mod;
   Simulation sim(mod);
   Parser & p = Parser::globalParser();
   p.parse("test_sun.yaml", mod, sim); p.postParse();

   RegularUpdateComponent & clock1 = mod.newComponent<RegularUpdateComponent>("clock1");
   clock1.setDt(posix_time::minutes(10));

   ofstream outfile;
   outfile.open("test_sun.out");

   clock1.didCompleteTimestep().addAction([&]() 
         {
            SphericalAngles sunCoords = sunPos(utcTime(clock1.time()), mod.latLong());
            outfile << dSeconds(clock1.time() - sim.startTime()) / (24 * 3600) << " " << sunCoords.zenith << " "
                    << solarPower(sunCoords, {0.0, 0.0}, 1.0) << std::endl;
         }, "clock1 update");

   mod.validate();
   sim.initialize();
   while (sim.doNextUpdate())
   {
   }
   outfile.close();
}

BOOST_AUTO_TEST_CASE (test_solar_pv)
{
   Model mod;
   Simulation sim(mod);
   Parser & p = Parser::globalParser();
   p.parse("test_solar_pv.yaml", mod, sim); p.postParse();

   SolarPv * spv2 = mod.component<SolarPv>("solar_pv_bus_2");
   InverterBase * inv2 = mod.component<InverterBase>("inverter_bus_2");
   Bus * bus2 = mod.component<Bus>("bus_2");
   Network * network = mod.component<Network>("network_1");

   ofstream outfile;
   outfile.open("test_solar_pv.out");

   network->didCompleteTimestep().addAction([&]()
         {
            outfile << dSeconds(sim.currentTime()-sim.startTime()) / 3600 << " " << spv2->PDc() << " " << inv2->S()(0)
                    << " " << bus2->V()(0) << std::endl;
         }, "Network updated.");
   mod.validate();
   sim.initialize();

   while (sim.doNextUpdate())
   {
   }
   outfile.close();
}

BOOST_AUTO_TEST_CASE (test_loops)
{
   Model mod;
   Simulation sim(mod);
   Parser & p = Parser::globalParser();
   p.parse("test_loops.yaml", mod, sim); p.postParse();

   SolarPv * spv2 = mod.component<SolarPv>("solar_pv_bus_2");
   InverterBase * inv2 = mod.component<InverterBase>("inverter_bus_2_0");
   Bus * bus2 = mod.component<Bus>("bus_2_1");
   Network * network = mod.component<Network>("network_1");

   ofstream outfile;
   outfile.open("test_loops.out");

   network->didCompleteTimestep().addAction([&]()
         {
            outfile << dSeconds(sim.currentTime()-sim.startTime()) / 3600 << " " << spv2->PDc() << " " << inv2->S()(0)
                    << " " << bus2->V()(0) << std::endl;
            std::cout << "timestep " << sim.currentTime()-sim.startTime() << std::endl;
         }, "Network updated.");
   mod.validate();
   sim.initialize();

   while (sim.doNextUpdate())
   {
   }
   outfile.close();
}

static void prepareMPInput(const std::string & yamlName, const std::string & caseName, bool usePerUnit)
{
   std::fstream yamlFile(yamlName, ios_base::out);
   if (!yamlFile.is_open())
   {
      Log().error() << "Could not open the yaml output file " << yamlName << "." << std::endl;
      SmartGridToolbox::abort();
   }

   yamlFile << "configuration_name:           config_1" << std::endl;
   yamlFile << "start_time:                   2013-01-23 13:13:00" << std::endl;
   yamlFile << "end_time:                     2013-01-23 15:13:00" << std::endl;
   yamlFile << "lat_long:                     [-35.3075, 149.1244] # Canberra, Australia." << std::endl;
   yamlFile << "timezone:                     AEST10AEDT,M10.5.0/02,M3.5.0/03 # Timezone info for Canberra, Australia."
            << std::endl;
   yamlFile << "components:" << std::endl;
   yamlFile << "   matpower:" << std::endl;
   yamlFile << "      input_file:             " << caseName << std::endl;
   yamlFile << "      network_name:           matpower" << std::endl;
   yamlFile << "      default_V_base:         1000 # 1 KV default." << std::endl;
   if (usePerUnit)
   {
      yamlFile << "      use_per_unit:           Y" << std::endl;
   }

   yamlFile.close();
}

static void readMPOutput(const std::string & fileName, bool usePerUnit, 
                         double & SBase, ublas::vector<int> & iBus, ublas::vector<double> & VBase, 
                         ublas::vector<Complex> & V, ublas::vector<Complex> & Sc, ublas::vector<Complex> & Sg)
{
   std::fstream infile(fileName);
   if (!infile.is_open())
   {
      Log().error() << "Could not open the matpower compare file " << fileName << "." << std::endl;
      SmartGridToolbox::abort();
   }
   infile >> SBase;
   int nBus;
   infile >> nBus;
   Log().message() << "Matpower output: nBus = " << nBus << std::endl;
   iBus.resize(nBus, false);
   VBase.resize(nBus, false);
   V.resize(nBus, false);
   Sc.resize(nBus, false);
   Sg.resize(nBus, false);
   for (int i = 0; i < nBus; ++i)
   {
      int ib;
      double Vb;
      double Vr; double Vi;
      double Scr; double Sci;
      double Sgr; double Sgi;
      infile >> ib >> Vb >> Vr >> Vi >> Scr >> Sci >> Sgr >> Sgi;
      iBus(i) = ib;
      VBase(i) = Vb;
      V(i) = Complex(Vr, Vi);
      Sc(i) = Complex(Scr, Sci);
      Sg(i) = Complex(Sgr, Sgi);
   }

   if (!usePerUnit)
   {
      V = element_prod(VBase, V);
      Sc *= SBase;
      Sg *= SBase;
   }
}

static std::string num2PaddedString5(int num)
{
   std::ostringstream ss;
   ss << std::setfill('0') << std::setw(5) << num;
   return ss.str();
}

static void testMatpower(const std::string & baseName, bool usePerUnit)
{
   std::string caseName = baseName + ".m";
   std::string yamlName = "test_mp_" + baseName + ".yaml";
   std::string compareName = "test_mp_" + baseName + ".compare";

   Model mod;
   Simulation sim(mod);
   Parser & p = Parser::globalParser();
   
   prepareMPInput(yamlName, caseName, usePerUnit);

   p.parse(yamlName.c_str(), mod, sim); p.postParse();
   mod.validate();
   sim.initialize();
   Network * network = mod.component<Network>("matpower");
   network->solvePowerFlow();

   double SBase;
   ublas::vector<int> iBus;
   ublas::vector<double> VBase;
   ublas::vector<Complex> V;
   ublas::vector<Complex> Sc;
   ublas::vector<Complex> Sg;
   readMPOutput(compareName, usePerUnit, SBase, iBus, VBase, V, Sc, Sg);

   double STol = usePerUnit ? 1e-4 : SBase * 1e-4;

   for (int i = 0; i < iBus.size(); ++i)
   {
      int ib = iBus(i);
      std::string busName = "matpower_bus_" + num2PaddedString5(ib);
      Bus * bus = mod.component<Bus>(busName);
      assert(bus != nullptr);
      double VTol = usePerUnit ? 1e-4 : VBase(i) * 1e-4;

      Log().message() << "V tolerance = " << VTol << std::endl;
      Log().message() << "S tolerance = " << STol << std::endl;
      Log().message() << std::endl;

      Log().message() << setw(24) << std::left << busName
                << setw(24) << std::left << "V"
                << setw(24) << std::left << "Sc"
                << setw(24) << std::left << "Sg"
                << std::endl;
      Log().message() << setw(24) << left << "SGT"
                << setw(24) << left << bus->V()(0)
                << setw(24) << left << bus->Sc()(0)
                << setw(24) << left << bus->Sg()(0)
                << std::endl;
      Log().message() << setw(24) << left << "Matpower"
                << setw(24) << left << V(i) 
                << setw(24) << left << Sc(i)
                << setw(24) << left << Sg(i)
                << std::endl; 
      Log().message() << std::endl;

      BOOST_CHECK(abs(bus->V()(0) - V(i)) < VTol);
      BOOST_CHECK(abs(bus->Sc()(0) - Sc(i)) < STol);
      BOOST_CHECK(abs(bus->Sg()(0) - Sg(i)) < STol);
   }
}

BOOST_AUTO_TEST_CASE (test_mp_SLPQ)
{
   testMatpower("caseSLPQ", true);
   testMatpower("caseSLPQ", false);
}

BOOST_AUTO_TEST_CASE (test_mp_SLPV)
{
   testMatpower("caseSLPV", true);
   testMatpower("caseSLPV", false);
}

BOOST_AUTO_TEST_CASE (test_mp_SLPQPV)
{
   testMatpower("caseSLPQPV", true);
   testMatpower("caseSLPQPV", false);
}

BOOST_AUTO_TEST_CASE (test_mp_4gs)
{
   testMatpower("case4gs", true);
   testMatpower("case4gs", false);
}

BOOST_AUTO_TEST_CASE (test_mp_6ww)
{
   testMatpower("case6ww", true);
   testMatpower("case6ww", false);
}

BOOST_AUTO_TEST_CASE (test_mp_9)
{
   testMatpower("case9", true);
   testMatpower("case9", false);
}

BOOST_AUTO_TEST_CASE (test_mp_trans3)
{
   testMatpower("case_trans3", true);
   testMatpower("case_trans3", false);
}

BOOST_AUTO_TEST_CASE (test_mp_trans3_shift)
{
   testMatpower("case_trans3_shift", true);
   testMatpower("case_trans3_shift", false);
}

BOOST_AUTO_TEST_CASE (test_mp_14)
{
   testMatpower("case14", true);
   testMatpower("case14", false);
}

BOOST_AUTO_TEST_CASE (test_mp_debug)
{
   testMatpower("case_debug", true);
}

BOOST_AUTO_TEST_CASE (test_mp_14_shift)
{
   testMatpower("case14_shift", true);
   testMatpower("case14_shift", false);
}

BOOST_AUTO_TEST_CASE (test_mp_57)
{
   testMatpower("case57", true);
   testMatpower("case57", false);
}

BOOST_AUTO_TEST_CASE (test_network_overhead)
{
   Model mod;
   Simulation sim(mod);

   Parser & p = Parser::globalParser();
   p.parse("test_network_overhead.yaml", mod, sim); p.postParse();

   Bus * bus1 = mod.component<Bus>("bus_1");
   Bus * bus2 = mod.component<Bus>("bus_2");

   mod.validate();
   sim.initialize();

   Network * network = mod.component<Network>("network_1");
   network->solvePowerFlow();

   Log().message() << bus1->V()(0) << " " << bus1->V()(1) << " " << bus1->V()(2) << std::endl;
   Log().message() << bus2->V()(0) << " " << bus2->V()(1) << " " << bus2->V()(2) <<  std::endl;
   Log().message() << std::endl;

   Log().message() << abs(bus1->V()(0)) << " " << abs(bus1->V()(1)) << " " << abs(bus1->V()(2)) <<  std::endl;
   Log().message() << abs(bus2->V()(0)) << " " << abs(bus2->V()(1)) << " " << abs(bus2->V()(2)) <<  std::endl;
   Log().message() << std::endl;

   Log().message() << bus1->STot()(0) << " " << bus1->STot()(1) << " " << bus1->STot()(2) << std::endl;
   Log().message() << bus2->STot()(0) << " " << bus2->STot()(1) << " " << bus2->STot()(2) <<  std::endl;
   Log().message() << std::endl;
}

static void prepareCDFInput(const std::string & yamlName, const std::string & caseName, bool usePerUnit)
{
   std::fstream yamlFile(yamlName, ios_base::out);
   if (!yamlFile.is_open())
   {
      Log().error() << "Could not open the yaml output file " << yamlName << "." << std::endl;
      SmartGridToolbox::abort();
   }

   yamlFile << "configuration_name:           config_1" << std::endl;
   yamlFile << "start_time:                   2013-01-23 13:13:00" << std::endl;
   yamlFile << "end_time:                     2013-01-23 15:13:00" << std::endl;
   yamlFile << "lat_long:                     [-35.3075, 149.1244] # Canberra, Australia." << std::endl;
   yamlFile << "timezone:                     AEST10AEDT,M10.5.0/02,M3.5.0/03 # Timezone info for Canberra, Australia."
            << std::endl;
   yamlFile << "components:" << std::endl;
   yamlFile << "   cdf:" << std::endl;
   yamlFile << "      input_file:             " << caseName << std::endl;
   yamlFile << "      network_name:           cdf" << std::endl;
   yamlFile << "      default_V_base:         1000 # 1 KV default." << std::endl;
   if (usePerUnit)
   {
      yamlFile << "      use_per_unit:           Y" << std::endl;
   }

   yamlFile.close();
}

static void testCDF(const std::string & baseName, bool usePerUnit)
{
   std::string caseName = baseName + ".cdf";
   std::string yamlName = "test_cdf_" + baseName + ".yaml";
   std::string compareName = "test_cdf_" + baseName + ".compare";

   Model mod;
   Simulation sim(mod);
   Parser & p = Parser::globalParser();
   
   prepareCDFInput(yamlName, caseName, usePerUnit);

   p.parse(yamlName.c_str(), mod, sim); p.postParse();
   mod.validate();
   sim.initialize();
   Network * network = mod.component<Network>("cdf");
   network->solvePowerFlow();

   double SBase;
   ublas::vector<int> iBus;
   ublas::vector<double> VBase;
   ublas::vector<Complex> V;
   ublas::vector<Complex> Sc;
   ublas::vector<Complex> Sg;
   readMPOutput(compareName, usePerUnit, SBase, iBus, VBase, V, Sc, Sg);

   double STol = usePerUnit ? 1e-4 : SBase * 1e-4;

   for (int i = 0; i < iBus.size(); ++i)
   {
      int ib = iBus(i);
      std::string busName = "cdf_bus_" + num2PaddedString5(ib);
      Bus * bus = mod.component<Bus>(busName);
      assert(bus != nullptr);
      double VTol = usePerUnit ? 1e-4 : VBase(i) * 1e-4;

      Log().message() << "V tolerance = " << VTol << std::endl;
      Log().message() << "S tolerance = " << STol << std::endl;
      Log().message() << std::endl;

      Log().message() << setw(24) << std::left << busName
                << setw(24) << std::left << "V"
                << setw(24) << std::left << "Sc"
                << setw(24) << std::left << "Sg"
                << std::endl;
      Log().message() << setw(24) << left << "SGT"
                << setw(24) << left << bus->V()(0)
                << setw(24) << left << bus->Sc()(0)
                << setw(24) << left << bus->Sg()(0)
                << std::endl;
      Log().message() << setw(24) << left << "Matpower"
                << setw(24) << left << V(i) 
                << setw(24) << left << Sc(i)
                << setw(24) << left << Sg(i)
                << std::endl; 
      Log().message() << std::endl;

      BOOST_CHECK(abs(bus->V()(0) - V(i)) < VTol);
      BOOST_CHECK(abs(bus->Sc()(0) - Sc(i)) < STol);
      BOOST_CHECK(abs(bus->Sg()(0) - Sg(i)) < STol);
   }
}

BOOST_AUTO_TEST_CASE (test_cdf_14)
{
   testCDF("ieee14", true);
   testCDF("ieee14", false);
}

BOOST_AUTO_TEST_CASE (test_transformers)
{
   Model mod;
   Simulation sim(mod);

   Parser & p = Parser::globalParser();
   p.parse("test_transformers.yaml", mod, sim); p.postParse();

   mod.validate();
   sim.initialize();

   Bus * bus1 = mod.component<Bus>("bus_1");
   Bus * bus2 = mod.component<Bus>("bus_2");
   Bus * bus3 = mod.component<Bus>("bus_3");

   Network * network = mod.component<Network>("network_1");
   Log().message() << *network << std::endl;
   network->solvePowerFlow();

   Log().message() << bus1->V()(0) << " " << bus1->V()(1) << " " << bus1->V()(2) << std::endl;
   Log().message() << bus2->V()(0) << " " << bus2->V()(1) << " " << bus2->V()(2) <<  std::endl;
   Log().message() << bus3->V()(0) << " " << bus3->V()(1) << " " << bus3->V()(2) <<  std::endl;
   Log().message() << std::endl;

   Log().message() << abs(bus1->V()(0)) << " " << abs(bus1->V()(1)) << " " << abs(bus1->V()(2)) <<  std::endl;
   Log().message() << abs(bus2->V()(0)) << " " << abs(bus2->V()(1)) << " " << abs(bus2->V()(2)) <<  std::endl;
   Log().message() << abs(bus3->V()(0)) << " " << abs(bus3->V()(1)) << " " << abs(bus3->V()(2)) <<  std::endl;
   Log().message() << std::endl;

   Log().message() << bus1->STot()(0) << " " << bus1->STot()(1) << " " << bus1->STot()(2) << std::endl;
   Log().message() << bus2->STot()(0) << " " << bus2->STot()(1) << " " << bus2->STot()(2) <<  std::endl;
   Log().message() << bus3->STot()(0) << " " << bus3->STot()(1) << " " << bus3->STot()(2) <<  std::endl;
   Log().message() << std::endl;
}

#endif

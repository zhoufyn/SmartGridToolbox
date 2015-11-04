// Copyright 2015 National ICT Australia Limited (NICTA)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "BuildingController.h"

#include "Building.h"

#include <stdlib.h>

namespace Sgt
{
    BuildingController::BuildingController(const std::string& id, const Time& dt) :
        Component(id), Heartbeat(id, dt) 
    {
        int error = GRBloadenv(&env, "");
        sgtAssert(error == 0, "Gurobi exited with error " << error);
        error = GRBsetintparam(env, "LogToConsole", 0);
        sgtAssert(error == 0, "Gurobi exited with error " << error);
    }

    void BuildingController::setBatt(std::shared_ptr<Battery> batt)
    {
        batt_ = batt;
        SimComponent::addDependency(*batt, *this, false);
    }

    void BuildingController::setBuild(std::shared_ptr<Building> build)
    {
        build_ = build;
        SimComponent::addDependency(*build, *this, false);
    }
            
    void BuildingController::setSolar(std::shared_ptr<SolarPv> solar)
    {
        solar_ = solar;
        SimComponent::addDependency(*solar, *this, false);
    }

    void BuildingController::updateState(Time t)
    {
        assert(batt_->validUntil() > t); // Next update is in the future.
        assert(build_->validUntil() > t); // Next update is in the future.
        assert(solar_->validUntil() > t); // Next update is in the future.

        // Variables:
        // [0 ... N - 1] : PImp[i] [-Inf, Inf]
        // [N ... 2N - 1] : PExp[i] [-Inf, Inf]
        // [2N ... 3N - 1] : PChg[i] [0, maxPChg]
        // [3N ... 4N - 1] : PDis[i] [0, maxPDis]
        // [4N ... 5N - 1] : Chg[i] [0, maxChg]
        // [5N ... 6N - 1] : Tb[i] [-Infin, Infin]
        // [6N ... 7N - 1] : TbPlus[i] [0, TMaxDev_]
        // [7N ... 8N - 1] : TbMinus[i] [0, TMaxDev_]
        // [8N ... 9N - 1] : TbPlusPen[i] [0, Infin]
        // [9N ... 10N - 1] : TbMinusPen[i] [0, Infin]
        // [10N ... 11N - 1] : PCool[i] [0, maxPCool]
        // [11N ... 12N - 1] : PHeat[i] [0, maxPHeat]
        //
        // Constraints:
        // Chg[0] = Chg0
        // Chg[i+1] - Chg[i] - chg_eff * dt * PChg[i] + (1 / dis_eff) * dt * PDis[i] = 0
        // Tb[0] = Tb0
        // Tb[i+1] - d Tb[i] + ((1 - d) COPCool / kb) PCool[i] - ((1 - d) COPHeat / kb) PHeat = (1 - d) TExt
        // Tb[i] - TbPlus[i] + TbMinus[i] - TbPlusPen + TbMinusPen = TSetp_
        // PImp[i] + PDis[i] - PExp[i] - PChg[i] - PHeat[i] - PCool[i] = PUncontrolledLoad[i]
        //
        // Objective:
        // Sum_i (price[i] * PImp[i] - feedInTariff_ * PExp[i] + comfortFactor_ * (TbPlusPen[i] + TbMinusPen[i])
        
        // Parameters:
        const int N = 100;
        const int dtSecs = 15 * 60; // 15 minute steps.
        const double dtHrs = dtSecs / 3600.0;

        // Variable Indexing
        const int nVar = 12 * N;

        int iPImp[N];
        int iPExp[N];
        int iPChg[N];
        int iPDis[N];
        int iChg[N];
        int iTb[N];
        int iTbPlus[N];
        int iTbMinus[N];
        int iTbPlusPen[N];
        int iTbMinusPen[N];
        int iPCool[N];
        int iPHeat[N];
        for (int i = 0; i < N; ++i)
        {
            iPImp[i] = i;
            iPExp[i] = N + i;
            iPChg[i] = 2 * N + i;
            iPDis[i] = 3 * N + i;
            iChg[i] = 4 * N + i;
            iTb[i] = 5 * N + i;
            iTbPlus[i] = 6 * N + i;
            iTbMinus[i] = 7 * N + i;
            iTbPlusPen[i] = 8 * N + i;
            iTbMinusPen[i] = 9 * N + i;
            iPCool[i] = 10 * N + i;
            iPHeat[i] = 11 * N + i;
        }

        // Constants:
        std::vector<double> PUncontrolledLoad; PUncontrolledLoad.reserve(N); // Solar and load, as a draw.
        std::vector<double> price; price.reserve(N);
        std::vector<double> TExt; TExt.reserve(N);
        for (int i = 0; i < N; ++i)
        {
            Time ti = t + timeFromDSeconds((i + 0.5) * dtSecs); // Use midpoint of timestep for consts.
            PUncontrolledLoad.push_back(-(solar_->PDc(ti) + real(loadSeries_->value(ti)[2])));
                // Change from inj. to draw.
            price.push_back(priceSeries_->value(ti));
            TExt.push_back(TExtSeries_->value(ti));
        }

        double chg0 = batt_->charge();
        
        double Tb0 = build_->Tb();
        
        // Gurobi:
        GRBmodel* model = NULL;
        int error = 0;

        double obj[nVar];
        double lb[nVar];
        double ub[nVar];
        char vtype[nVar];
        char varnames1[nVar][32];
        char* varnames[nVar];

        for (size_t i = 0; i < N; ++i)
        {
            // PImp:
            obj[iPImp[i]] = price[i];
            lb[iPImp[i]] = 0.0;
            ub[iPImp[i]] = INFINITY;
            vtype[iPImp[i]] = GRB_CONTINUOUS;
            sprintf(varnames1[iPImp[i]], "P_imp_%lu", i);
            varnames[iPImp[i]] = varnames1[iPImp[i]];
            
            // PExp:
            obj[iPExp[i]] = -feedInTariff_;
            lb[iPExp[i]] = 0.0;
            ub[iPExp[i]] = INFINITY;
            vtype[iPExp[i]] = GRB_CONTINUOUS;
            sprintf(varnames1[iPExp[i]], "P_exp_%lu", i);
            varnames[iPExp[i]] = varnames1[iPExp[i]];

            // PChg:
            obj[iPChg[i]] = 0.0;
            lb[iPChg[i]] = 0.0;
            ub[iPChg[i]] = batt_->maxChargePower();
            vtype[iPChg[i]] = GRB_CONTINUOUS;
            sprintf(varnames1[iPChg[i]], "P_chg_%lu", i);
            varnames[iPChg[i]] = varnames1[iPChg[i]];

            // PDis:
            obj[iPDis[i]] = 0.0;
            lb[iPDis[i]] = 0.0;
            ub[iPDis[i]] = batt_->maxDischargePower();
            vtype[iPDis[i]] = GRB_CONTINUOUS;
            sprintf(varnames1[iPDis[i]], "P_dis_%lu", i);
            varnames[iPDis[i]] = varnames1[iPDis[i]];
            
            // Chg:
            obj[iChg[i]] = 0.0;
            lb[iChg[i]] = 0.0;
            ub[iChg[i]] = batt_->maxCharge();
            vtype[iChg[i]] = GRB_CONTINUOUS;
            sprintf(varnames1[iChg[i]], "chg_%lu", i);
            varnames[iChg[i]] = varnames1[iChg[i]];
            
            // Tb:
            obj[iTb[i]] = 0.0;
            lb[iTb[i]] = -INFINITY;
            ub[iTb[i]] = INFINITY;
            vtype[iTb[i]] = GRB_CONTINUOUS;
            sprintf(varnames1[iTb[i]], "Tb_%lu", i);
            varnames[iTb[i]] = varnames1[iTb[i]];

            // TbPlus:
            obj[iTbPlus[i]] = 0.0;
            lb[iTbPlus[i]] = 0.0;
            ub[iTbPlus[i]] = TMaxDev_;
            vtype[iTbPlus[i]] = GRB_CONTINUOUS;
            sprintf(varnames1[iTbPlus[i]], "TbPlus_%lu", i);
            varnames[iTbPlus[i]] = varnames1[iTbPlus[i]];

            // TbMinus:
            obj[iTbMinus[i]] = 0.0;
            lb[iTbMinus[i]] = 0.0;
            ub[iTbMinus[i]] = TMaxDev_;
            vtype[iTbMinus[i]] = GRB_CONTINUOUS;
            sprintf(varnames1[iTbMinus[i]], "TbMinus_%lu", i);
            varnames[iTbMinus[i]] = varnames1[iTbMinus[i]];
 
            // TbPlusPen:
            obj[iTbPlusPen[i]] = comfortFactor_;
            lb[iTbPlusPen[i]] = 0.0;
            ub[iTbPlusPen[i]] = INFINITY;
            vtype[iTbPlusPen[i]] = GRB_CONTINUOUS;
            sprintf(varnames1[iTbPlusPen[i]], "TbPlusPen_%lu", i);
            varnames[iTbPlusPen[i]] = varnames1[iTbPlusPen[i]];

            // TbMinus:
            obj[iTbMinusPen[i]] = comfortFactor_;
            lb[iTbMinusPen[i]] = 0.0;
            ub[iTbMinusPen[i]] = INFINITY;
            vtype[iTbMinusPen[i]] = GRB_CONTINUOUS;
            sprintf(varnames1[iTbMinusPen[i]], "TbMinusPen_%lu", i);
            varnames[iTbMinusPen[i]] = varnames1[iTbMinusPen[i]];
                      
            // PCool:
            obj[iPCool[i]] = 0.0;
            lb[iPCool[i]] = 0.0;
            ub[iPCool[i]] = build_->maxPCool();
            vtype[iPCool[i]] = GRB_CONTINUOUS;
            sprintf(varnames1[iPCool[i]], "PCool_%lu", i);
            varnames[iPCool[i]] = varnames1[iPCool[i]];
            
            // PHeat:
            obj[iPHeat[i]] = 0.0;
            lb[iPHeat[i]] = 0.0;
            ub[iPHeat[i]] = build_->maxPHeat();
            vtype[iPHeat[i]] = GRB_CONTINUOUS;
            sprintf(varnames1[iPHeat[i]], "PHeat_%lu", i);
            varnames[iPHeat[i]] = varnames1[iPHeat[i]];
        }

        error = GRBnewmodel(env, &model, "gurobi_model", nVar, obj, lb, ub, vtype, varnames);
        sgtAssert(error == 0, "Gurobi exited with error " << error);
        error = GRBsetintattr(model, "ModelSense", 1);
        sgtAssert(error == 0, "Gurobi exited with error " << error);
        error = GRBupdatemodel(model);
        sgtAssert(error == 0, "Gurobi exited with error " << error);

        // Add constraints:
        
        // Chg[0] = Chg0
        {
            int constrInds[] = {iChg[0]};
            double constrVals[] = {1.0};
            char buff[32]; sprintf(buff, "constr_chg_%d", 0);
            error = GRBaddconstr(model, 1, constrInds, constrVals, GRB_EQUAL, chg0, buff);
            sgtAssert(error == 0, "Gurobi exited with error " << error);
        }

        // Chg[i+1] - Chg[i] - chg_eff * dt * PChg[i] + (1 / dis_eff) * dt * PDis[i] = 0
        {
            double chgFactor = -dtHrs * batt_->chargeEfficiency();
            double disFactor = dtHrs / batt_->dischargeEfficiency();
            for (int i = 0; i < N - 1; ++i)
            {
                int constrInds[] = {iChg[i + 1], iChg[i], iPChg[i], iPDis[i]};
                double constrVals[] = {1.0, -1.0, chgFactor, disFactor};
                char buff[32]; sprintf(buff, "constr_chg_%d", i + 1);
                error = GRBaddconstr(model, 4, constrInds, constrVals, GRB_EQUAL, 0.0, buff);
                sgtAssert(error == 0, "Gurobi exited with error " << error);
            }
        }

        // Tb[0] = Tb0
        {
            int constrInds[] = {iTb[0]};
            double constrVals[] = {1.0};
            char buff[32]; sprintf(buff, "constr_Tb_%d", 0);
            error = GRBaddconstr(model, 1, constrInds, constrVals, GRB_EQUAL, Tb0, buff);
            sgtAssert(error == 0, "Gurobi exited with error " << error);
        }
        
        // Tb[i+1] - d Tb[i] + ((1 - d) COPCool / kb) PCool[i] - ((1 - d) COPHeat / kb) PHeat = (1 - d) TExt
        for (size_t i = 0; i < N - 1; ++i)
        {
            double d = build_->d(dtSecs);
            double d1 = 1.0 - d;
            double tmp = d1 / build_->kb();
            double PCoolFactor = tmp * build_->copCool();
            double PHeatFactor = -tmp * build_->copHeat();
            int constrInds[] = {iTb[i + 1], iTb[i], iPCool[i], iPHeat[i]};
            double constrVals[] = {1.0, -1.0, PCoolFactor, PHeatFactor};
            char buff[32]; sprintf(buff, "constr_Tb_%d", 0);
            error = GRBaddconstr(model, 4, constrInds, constrVals, GRB_EQUAL, d1 * TExt[i], buff);
            sgtAssert(error == 0, "Gurobi exited with error " << error);
        }
        
        // Tb[i] - TbPlus[i] + TbMinus[i] - TbPlusPen + TbMinusPen = TSetp_
        for (size_t i = 0; i < N; ++i)
        {
            int constrInds[] = {iTb[i], iTbPlus[i], iTbMinus[i], iTbPlusPen[i], iTbMinusPen[i]};
            double constrVals[] = {1.0, -1.0, 1.0, -1.0, 1.0};
            char buff[32]; sprintf(buff, "constr_Tb_bal_%d", 0);
            error = GRBaddconstr(model, 5, constrInds, constrVals, GRB_EQUAL, TSetp_, buff);
            sgtAssert(error == 0, "Gurobi exited with error " << error);
        }
        
        // PImp[i] + PDis[i] - PExp[i] - PChg[i] - PHeat[i] - PCool[i] = PUncontrolledLoad[i]
        for (size_t i = 0; i < N; ++i)
        {
            int constrInds[] = {iPImp[i], iPDis[i], iPExp[i], iPChg[i], iPHeat[i], iPCool[i]};
            double constrVals[] = {1.0, 1.0, -1.0, -1.0, -1.0, -1.0};
            char buff[32]; sprintf(buff, "constr_P_bal_%lu", i);
            error = GRBaddconstr(model, 6, constrInds, constrVals, GRB_EQUAL, PUncontrolledLoad[i], buff);
            sgtAssert(error == 0, "Gurobi exited with error " << error);
        }
       
        error = GRBupdatemodel(model);
        sgtAssert(error == 0, "Gurobi exited with error " << error);
        
        error = GRBoptimize(model);
        sgtAssert(error == 0, "Gurobi exited with error " << error);
        
        // Retrieve the results:
        double PImp[N];
        error = GRBgetdblattrarray(model, "X", 0, N, PImp);
        double PExp[N];
        error = GRBgetdblattrarray(model, "X", N, N, PExp);
        double PChg[N];
        error = GRBgetdblattrarray(model, "X", 2 * N, N, PChg);
        double PDis[N];
        error = GRBgetdblattrarray(model, "X", 3 * N, N, PDis);
        double chg[N];
        error = GRBgetdblattrarray(model, "X", 4 * N, N, chg);
        double Tb[N];
        error = GRBgetdblattrarray(model, "X", 5 * N, N, Tb);
        double TbPlus[N];
        error = GRBgetdblattrarray(model, "X", 6 * N, N, TbPlus);
        double TbMinus[N];
        error = GRBgetdblattrarray(model, "X", 7 * N, N, TbMinus);
        double TbPlusPen[N];
        error = GRBgetdblattrarray(model, "X", 8 * N, N, TbPlusPen);
        double TbMinusPen[N];
        error = GRBgetdblattrarray(model, "X", 9 * N, N, TbMinusPen);
        double PCool[N];
        error = GRBgetdblattrarray(model, "X", 10 * N, N, PCool);
        double PHeat[N];
        error = GRBgetdblattrarray(model, "X", 11 * N, N, PHeat);

        // Propogate control to battery and building:
        batt_->setRequestedPower(PDis[0] - PChg[0]); // Injection.
        assert(PCool[0] * PHeat[0] < std::numeric_limits<double>::epsilon());
        build_->setReqPCool(PCool[0]); // Injection.
        build_->setReqPHeat(PHeat[0]); // Injection.
        
        Heartbeat::updateState(t);
    }

    void BuildingControllerParserPlugin::parse(const YAML::Node& nd, Simulation& sim, const ParserBase& parser) const
    {
        string id = parser.expand<std::string>(nd["id"]);
        Time dt = posix_time::minutes(5);
        auto contr = sim.newSimComponent<BuildingController>(id, dt);
        
        auto ndDt = nd["dt"];
        if (ndDt)
        {
            dt = parser.expand<Time>(ndDt);
            contr->setDt(dt);
        }

        id = parser.expand<std::string>(nd["battery"]);
        contr->setBatt(sim.simComponent<Battery>(id));
        
        id = parser.expand<std::string>(nd["building"]);
        contr->setBuild(sim.simComponent<Building>(id));
        
        id = parser.expand<std::string>(nd["solar"]);
        contr->setSolar(sim.simComponent<SolarPv>(id));

        id = parser.expand<std::string>(nd["load_series"]);
        contr->setLoadSeries(sim.timeSeries<BuildingController::LoadSeries>(id));
        
        id = parser.expand<std::string>(nd["price_series"]);
        contr->setPriceSeries(sim.timeSeries<BuildingController::PriceSeries>(id));
        
        id = parser.expand<std::string>(nd["T_extern_series"]);
        contr->setTExtSeries(sim.timeSeries<BuildingController::TempSeries>(id));
        
        double feedInTariff = parser.expand<double>(nd["feed_in_tariff"]);
        contr->setFeedInTariff(feedInTariff);
        
        double comfortFactor = parser.expand<double>(nd["comfort_factor"]);
        contr->setComfortFactor(comfortFactor);
        
        double TSetp = parser.expand<double>(nd["T_setpoint"]);
        contr->setTSetp(TSetp);
        
        double TMaxDev = parser.expand<double>(nd["T_max_dev"]);
        contr->setTMaxDev(TMaxDev);
    }
}

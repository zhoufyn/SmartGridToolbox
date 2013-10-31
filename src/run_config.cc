#include "Common.h"
#include "Model.h"
#include "Parser.h"
#include "RegisterParserPlugins.h"
#include "Simulation.h"

using namespace SmartGridToolbox;

int main(int argc, const char ** argv)
{
   if (argc != 2)
   {
      error() << "Usage: " << argv[0] << " config_name" << std::endl;
      SmartGridToolbox::abort();
   }

   const char * configName = argv[1];

   Model mod;
   Simulation sim(mod);
   Parser & p = Parser::globalParser();
   p.parse(configName, mod, sim); p.postParse();
   mod.validate();
   sim.initialize();
   while (sim.doNextUpdate());
}

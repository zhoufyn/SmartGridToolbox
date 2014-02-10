#ifndef NETWORK_PARSER_DOT_H
#define NETWORK_PARSER_DOT_H

#include <SmartGridToolbox/Parser.h>

namespace SmartGridToolbox
{
   /// ParserPlugin that parses the network keyword, adding a new Network to the model.
   class NetworkParser : public ParserPlugin
   {
      public:
         static constexpr const char* pluginKey()
         {
            return "network";
         }

      public:
         virtual void parse(const YAML::Node & nd, Model & mod, const ParserState & state) const override;
   };
}

#endif // NETWORK_PARSER_DOT_H

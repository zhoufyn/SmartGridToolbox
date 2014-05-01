#ifndef COMPONENT_TEMPLATE_DOT_H
#define COMPONENT_TEMPLATE_DOT_H

#include <iostream>

namespace SmartGridToolbox
{
   class ComponentTemplate : public Component
   {
      public:
         /// @name Lifecycle.
         /// @{
         ComponentTemplate(std::string name);
         virtual ~ComponentTemplate();
         /// @}

      public:
         /// @name Public overridden member functions from Component.
         /// @{
         virtual Time validUntil() const override;
         /// @}

      protected:
         /// @name Protected overridden member functions from Component.
         /// @{
         virtual void initializeState() override;
         virtual void updateState(Time t) override;
         /// @}

      public:
         /// @name My public member functions.
         /// @{
         /// @}

      private:
         /// @name My private member functions.
         /// @{
         /// @}

      private:
         /// @name My private member data.
         /// @{
         /// @}
   }
}

#endif // COMPONENT_TEMPLATE_DOT_H
